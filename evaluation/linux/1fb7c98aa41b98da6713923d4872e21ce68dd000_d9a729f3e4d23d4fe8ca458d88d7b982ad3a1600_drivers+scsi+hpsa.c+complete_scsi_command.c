static void complete_scsi_command(struct CommandList *cp)
{
	struct scsi_cmnd *cmd;
	struct ctlr_info *h;
	struct ErrorInfo *ei;
	struct hpsa_scsi_dev_t *dev;
	struct io_accel2_cmd *c2;

	u8 sense_key;
	u8 asc;      /* additional sense code */
	u8 ascq;     /* additional sense code qualifier */
	unsigned long sense_data_size;

	ei = cp->err_info;
	cmd = cp->scsi_cmd;
	h = cp->h;
	dev = cmd->device->hostdata;
	c2 = &h->ioaccel2_cmd_pool[cp->cmdindex];

	scsi_dma_unmap(cmd); /* undo the DMA mappings */
	if ((cp->cmd_type == CMD_SCSI) &&
		(le16_to_cpu(cp->Header.SGTotal) > h->max_cmd_sg_entries))
		hpsa_unmap_sg_chain_block(h, cp);

	if ((cp->cmd_type == CMD_IOACCEL2) &&
		(c2->sg[0].chain_indicator == IOACCEL2_CHAIN))
		hpsa_unmap_ioaccel2_sg_chain_block(h, c2);

	cmd->result = (DID_OK << 16); 		/* host byte */
	cmd->result |= (COMMAND_COMPLETE << 8);	/* msg byte */

	if (cp->cmd_type == CMD_IOACCEL2 || cp->cmd_type == CMD_IOACCEL1)
		atomic_dec(&cp->phys_disk->ioaccel_cmds_out);

	/*
	 * We check for lockup status here as it may be set for
	 * CMD_SCSI, CMD_IOACCEL1 and CMD_IOACCEL2 commands by
	 * fail_all_oustanding_cmds()
	 */
	if (unlikely(ei->CommandStatus == CMD_CTLR_LOCKUP)) {
		/* DID_NO_CONNECT will prevent a retry */
		cmd->result = DID_NO_CONNECT << 16;
		cmd_free(h, cp);
		cmd->scsi_done(cmd);
		return;
	}

	if (cp->cmd_type == CMD_IOACCEL2)
		return process_ioaccel2_completion(h, cp, cmd, dev);

	scsi_set_resid(cmd, ei->ResidualCnt);
	if (ei->CommandStatus == 0) {
		if (cp->cmd_type == CMD_IOACCEL1)
			atomic_dec(&cp->phys_disk->ioaccel_cmds_out);
		cmd_free(h, cp);
		cmd->scsi_done(cmd);
		return;
	}

	/* For I/O accelerator commands, copy over some fields to the normal
	 * CISS header used below for error handling.
	 */
	if (cp->cmd_type == CMD_IOACCEL1) {
		struct io_accel1_cmd *c = &h->ioaccel_cmd_pool[cp->cmdindex];
		cp->Header.SGList = scsi_sg_count(cmd);
		cp->Header.SGTotal = cpu_to_le16(cp->Header.SGList);
		cp->Request.CDBLen = le16_to_cpu(c->io_flags) &
			IOACCEL1_IOFLAGS_CDBLEN_MASK;
		cp->Header.tag = c->tag;
		memcpy(cp->Header.LUN.LunAddrBytes, c->CISS_LUN, 8);
		memcpy(cp->Request.CDB, c->CDB, cp->Request.CDBLen);

		/* Any RAID offload error results in retry which will use
		 * the normal I/O path so the controller can handle whatever's
		 * wrong.
		 */
		if (is_logical_dev_addr_mode(dev->scsi3addr)) {
			if (ei->CommandStatus == CMD_IOACCEL_DISABLED)
				dev->offload_enabled = 0;
			INIT_WORK(&cp->work, hpsa_command_resubmit_worker);
			queue_work_on(raw_smp_processor_id(),
					h->resubmit_wq, &cp->work);
			return;
		}
	}

	/* an error has occurred */
	switch (ei->CommandStatus) {

	case CMD_TARGET_STATUS:
		cmd->result |= ei->ScsiStatus;
		/* copy the sense data */
		if (SCSI_SENSE_BUFFERSIZE < sizeof(ei->SenseInfo))
			sense_data_size = SCSI_SENSE_BUFFERSIZE;
		else
			sense_data_size = sizeof(ei->SenseInfo);
		if (ei->SenseLen < sense_data_size)
			sense_data_size = ei->SenseLen;
		memcpy(cmd->sense_buffer, ei->SenseInfo, sense_data_size);
		if (ei->ScsiStatus)
			decode_sense_data(ei->SenseInfo, sense_data_size,
				&sense_key, &asc, &ascq);
		if (ei->ScsiStatus == SAM_STAT_CHECK_CONDITION) {
			if (sense_key == ABORTED_COMMAND) {
				cmd->result |= DID_SOFT_ERROR << 16;
				break;
			}
			break;
		}
		/* Problem was not a check condition
		 * Pass it up to the upper layers...
		 */
		if (ei->ScsiStatus) {
			dev_warn(&h->pdev->dev, "cp %p has status 0x%x "
				"Sense: 0x%x, ASC: 0x%x, ASCQ: 0x%x, "
				"Returning result: 0x%x\n",
				cp, ei->ScsiStatus,
				sense_key, asc, ascq,
				cmd->result);
		} else {  /* scsi status is zero??? How??? */
			dev_warn(&h->pdev->dev, "cp %p SCSI status was 0. "
				"Returning no connection.\n", cp),

			/* Ordinarily, this case should never happen,
			 * but there is a bug in some released firmware
			 * revisions that allows it to happen if, for
			 * example, a 4100 backplane loses power and
			 * the tape drive is in it.  We assume that
			 * it's a fatal error of some kind because we
			 * can't show that it wasn't. We will make it
			 * look like selection timeout since that is
			 * the most common reason for this to occur,
			 * and it's severe enough.
			 */

			cmd->result = DID_NO_CONNECT << 16;
		}
		break;

	case CMD_DATA_UNDERRUN: /* let mid layer handle it. */
		break;
	case CMD_DATA_OVERRUN:
		dev_warn(&h->pdev->dev,
			"CDB %16phN data overrun\n", cp->Request.CDB);
		break;
	case CMD_INVALID: {
		/* print_bytes(cp, sizeof(*cp), 1, 0);
		print_cmd(cp); */
		/* We get CMD_INVALID if you address a non-existent device
		 * instead of a selection timeout (no response).  You will
		 * see this if you yank out a drive, then try to access it.
		 * This is kind of a shame because it means that any other
		 * CMD_INVALID (e.g. driver bug) will get interpreted as a
		 * missing target. */
		cmd->result = DID_NO_CONNECT << 16;
	}
		break;
	case CMD_PROTOCOL_ERR:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "CDB %16phN : protocol error\n",
				cp->Request.CDB);
		break;
	case CMD_HARDWARE_ERR:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "CDB %16phN : hardware error\n",
			cp->Request.CDB);
		break;
	case CMD_CONNECTION_LOST:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "CDB %16phN : connection lost\n",
			cp->Request.CDB);
		break;
	case CMD_ABORTED:
		cmd->result = DID_ABORT << 16;
		dev_warn(&h->pdev->dev, "CDB %16phN was aborted with status 0x%x\n",
				cp->Request.CDB, ei->ScsiStatus);
		break;
	case CMD_ABORT_FAILED:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "CDB %16phN : abort failed\n",
			cp->Request.CDB);
		break;
	case CMD_UNSOLICITED_ABORT:
		cmd->result = DID_SOFT_ERROR << 16; /* retry the command */
		dev_warn(&h->pdev->dev, "CDB %16phN : unsolicited abort\n",
			cp->Request.CDB);
		break;
	case CMD_TIMEOUT:
		cmd->result = DID_TIME_OUT << 16;
		dev_warn(&h->pdev->dev, "CDB %16phN timed out\n",
			cp->Request.CDB);
		break;
	case CMD_UNABORTABLE:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "Command unabortable\n");
		break;
	case CMD_TMF_STATUS:
		if (hpsa_evaluate_tmf_status(h, cp)) /* TMF failed? */
			cmd->result = DID_ERROR << 16;
		break;
	case CMD_IOACCEL_DISABLED:
		/* This only handles the direct pass-through case since RAID
		 * offload is handled above.  Just attempt a retry.
		 */
		cmd->result = DID_SOFT_ERROR << 16;
		dev_warn(&h->pdev->dev,
				"cp %p had HP SSD Smart Path error\n", cp);
		break;
	default:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "cp %p returned unknown status %x\n",
				cp, ei->CommandStatus);
	}
	cmd_free(h, cp);
	cmd->scsi_done(cmd);
}
