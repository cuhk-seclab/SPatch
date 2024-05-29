static void qlt_do_ctio_completion(struct scsi_qla_host *vha, uint32_t handle,
	uint32_t status, void *ctio)
{
	struct qla_hw_data *ha = vha->hw;
	struct se_cmd *se_cmd;
	const struct target_core_fabric_ops *tfo;
	struct qla_tgt_cmd *cmd;

	if (handle & CTIO_INTERMEDIATE_HANDLE_MARK) {
		/* That could happen only in case of an error/reset/abort */
		if (status != CTIO_SUCCESS) {
			ql_dbg(ql_dbg_tgt_mgt, vha, 0xf01d,
			    "Intermediate CTIO received"
			    " (status %x)\n", status);
		}
		return;
	}

	cmd = qlt_ctio_to_cmd(vha, handle, ctio);
	if (cmd == NULL)
		return;

	se_cmd = &cmd->se_cmd;
	tfo = se_cmd->se_tfo;
	cmd->cmd_sent_to_fw = 0;

	qlt_unmap_sg(vha, cmd);

	if (unlikely(status != CTIO_SUCCESS)) {
		switch (status & 0xFFFF) {
		case CTIO_LIP_RESET:
		case CTIO_TARGET_RESET:
		case CTIO_ABORTED:
			/* driver request abort via Terminate exchange */
		case CTIO_TIMEOUT:
		case CTIO_INVALID_RX_ID:
			/* They are OK */
			ql_dbg(ql_dbg_tgt_mgt, vha, 0xf058,
			    "qla_target(%d): CTIO with "
			    "status %#x received, state %x, se_cmd %p, "
			    "(LIP_RESET=e, ABORTED=2, TARGET_RESET=17, "
			    "TIMEOUT=b, INVALID_RX_ID=8)\n", vha->vp_idx,
			    status, cmd->state, se_cmd);
			break;

		case CTIO_PORT_LOGGED_OUT:
		case CTIO_PORT_UNAVAILABLE:
			ql_dbg(ql_dbg_tgt_mgt, vha, 0xf059,
			    "qla_target(%d): CTIO with PORT LOGGED "
			    "OUT (29) or PORT UNAVAILABLE (28) status %x "
			    "received (state %x, se_cmd %p)\n", vha->vp_idx,
			    status, cmd->state, se_cmd);
			break;

		case CTIO_SRR_RECEIVED:
			ql_dbg(ql_dbg_tgt_mgt, vha, 0xf05a,
			    "qla_target(%d): CTIO with SRR_RECEIVED"
			    " status %x received (state %x, se_cmd %p)\n",
			    vha->vp_idx, status, cmd->state, se_cmd);
			if (qlt_prepare_srr_ctio(vha, cmd, ctio) != 0)
				break;
			else
				return;

		case CTIO_DIF_ERROR: {
			struct ctio_crc_from_fw *crc =
				(struct ctio_crc_from_fw *)ctio;
			ql_dbg(ql_dbg_tgt_mgt, vha, 0xf073,
			    "qla_target(%d): CTIO with DIF_ERROR status %x received (state %x, se_cmd %p) actual_dif[0x%llx] expect_dif[0x%llx]\n",
			    vha->vp_idx, status, cmd->state, se_cmd,
			    *((u64 *)&crc->actual_dif[0]),
			    *((u64 *)&crc->expected_dif[0]));

			if (qlt_handle_dif_error(vha, cmd, ctio)) {
				if (cmd->state == QLA_TGT_STATE_NEED_DATA) {
					/* scsi Write/xfer rdy complete */
					goto skip_term;
				} else {
					/* scsi read/xmit respond complete
					 * call handle dif to send scsi status
					 * rather than terminate exchange.
					 */
					cmd->state = QLA_TGT_STATE_PROCESSED;
					ha->tgt.tgt_ops->handle_dif_err(cmd);
					return;
				}
			} else {
				/* Need to generate a SCSI good completion.
				 * because FW did not send scsi status.
				 */
				status = 0;
				goto skip_term;
			}
			break;
		}
		default:
			ql_dbg(ql_dbg_tgt_mgt, vha, 0xf05b,
			    "qla_target(%d): CTIO with error status 0x%x received (state %x, se_cmd %p\n",
			    vha->vp_idx, status, cmd->state, se_cmd);
			break;
		}


		/* "cmd->state == QLA_TGT_STATE_ABORTED" means
		 * cmd is already aborted/terminated, we don't
		 * need to terminate again.  The exchange is already
		 * cleaned up/freed at FW level.  Just cleanup at driver
		 * level.
		 */
		if ((cmd->state != QLA_TGT_STATE_NEED_DATA) &&
		    (cmd->state != QLA_TGT_STATE_ABORTED)) {
			cmd->cmd_flags |= BIT_13;
			if (qlt_term_ctio_exchange(vha, ctio, cmd, status))
				return;
		}
	}
skip_term:

	if (cmd->state == QLA_TGT_STATE_PROCESSED) {
		;
	} else if (cmd->state == QLA_TGT_STATE_NEED_DATA) {
		int rx_status = 0;

		cmd->state = QLA_TGT_STATE_DATA_IN;

		if (unlikely(status != CTIO_SUCCESS))
			rx_status = -EIO;
		else
			cmd->write_data_transferred = 1;

		ha->tgt.tgt_ops->handle_data(cmd);
		return;
	} else if (cmd->state == QLA_TGT_STATE_ABORTED) {
		ql_dbg(ql_dbg_tgt_mgt, vha, 0xf01e,
		    "Aborted command %p (tag %d) finished\n", cmd, cmd->tag);
	} else {
		ql_dbg(ql_dbg_tgt_mgt, vha, 0xf05c,
		    "qla_target(%d): A command in state (%d) should "
		    "not return a CTIO complete\n", vha->vp_idx, cmd->state);
	}

	if (unlikely(status != CTIO_SUCCESS) &&
		(cmd->state != QLA_TGT_STATE_ABORTED)) {
		ql_dbg(ql_dbg_tgt_mgt, vha, 0xf01f, "Finishing failed CTIO\n");
		dump_stack();
	}


	ha->tgt.tgt_ops->free_cmd(cmd);
}
