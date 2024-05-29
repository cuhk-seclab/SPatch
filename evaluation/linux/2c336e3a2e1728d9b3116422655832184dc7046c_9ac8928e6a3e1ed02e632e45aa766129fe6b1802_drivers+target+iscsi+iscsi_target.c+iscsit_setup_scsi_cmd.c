int iscsit_setup_scsi_cmd(struct iscsi_conn *conn, struct iscsi_cmd *cmd,
			  unsigned char *buf)
{
	int data_direction, payload_length;
	struct iscsi_scsi_req *hdr;
	int iscsi_task_attr;
	int sam_task_attr;

	atomic_long_inc(&conn->sess->cmd_pdus);

	hdr			= (struct iscsi_scsi_req *) buf;
	payload_length		= ntoh24(hdr->dlength);

	/* FIXME; Add checks for AdditionalHeaderSegment */

	if (!(hdr->flags & ISCSI_FLAG_CMD_WRITE) &&
	    !(hdr->flags & ISCSI_FLAG_CMD_FINAL)) {
		pr_err("ISCSI_FLAG_CMD_WRITE & ISCSI_FLAG_CMD_FINAL"
				" not set. Bad iSCSI Initiator.\n");
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_BOOKMARK_INVALID, buf);
	}

	if (((hdr->flags & ISCSI_FLAG_CMD_READ) ||
	     (hdr->flags & ISCSI_FLAG_CMD_WRITE)) && !hdr->data_length) {
		/*
		 * From RFC-3720 Section 10.3.1:
		 *
		 * "Either or both of R and W MAY be 1 when either the
		 *  Expected Data Transfer Length and/or Bidirectional Read
		 *  Expected Data Transfer Length are 0"
		 *
		 * For this case, go ahead and clear the unnecssary bits
		 * to avoid any confusion with ->data_direction.
		 */
		hdr->flags &= ~ISCSI_FLAG_CMD_READ;
		hdr->flags &= ~ISCSI_FLAG_CMD_WRITE;

		pr_warn("ISCSI_FLAG_CMD_READ or ISCSI_FLAG_CMD_WRITE"
			" set when Expected Data Transfer Length is 0 for"
			" CDB: 0x%02x, Fixing up flags\n", hdr->cdb[0]);
	}

	if (!(hdr->flags & ISCSI_FLAG_CMD_READ) &&
	    !(hdr->flags & ISCSI_FLAG_CMD_WRITE) && (hdr->data_length != 0)) {
		pr_err("ISCSI_FLAG_CMD_READ and/or ISCSI_FLAG_CMD_WRITE"
			" MUST be set if Expected Data Transfer Length is not 0."
			" Bad iSCSI Initiator\n");
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_BOOKMARK_INVALID, buf);
	}

	if ((hdr->flags & ISCSI_FLAG_CMD_READ) &&
	    (hdr->flags & ISCSI_FLAG_CMD_WRITE)) {
		pr_err("Bidirectional operations not supported!\n");
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_BOOKMARK_INVALID, buf);
	}

	if (hdr->opcode & ISCSI_OP_IMMEDIATE) {
		pr_err("Illegally set Immediate Bit in iSCSI Initiator"
				" Scsi Command PDU.\n");
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_BOOKMARK_INVALID, buf);
	}

	if (payload_length && !conn->sess->sess_ops->ImmediateData) {
		pr_err("ImmediateData=No but DataSegmentLength=%u,"
			" protocol error.\n", payload_length);
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_PROTOCOL_ERROR, buf);
	}

	if ((be32_to_cpu(hdr->data_length) == payload_length) &&
	    (!(hdr->flags & ISCSI_FLAG_CMD_FINAL))) {
		pr_err("Expected Data Transfer Length and Length of"
			" Immediate Data are the same, but ISCSI_FLAG_CMD_FINAL"
			" bit is not set protocol error\n");
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_PROTOCOL_ERROR, buf);
	}

	if (payload_length > be32_to_cpu(hdr->data_length)) {
		pr_err("DataSegmentLength: %u is greater than"
			" EDTL: %u, protocol error.\n", payload_length,
				hdr->data_length);
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_PROTOCOL_ERROR, buf);
	}

	if (payload_length > conn->conn_ops->MaxXmitDataSegmentLength) {
		pr_err("DataSegmentLength: %u is greater than"
			" MaxXmitDataSegmentLength: %u, protocol error.\n",
			payload_length, conn->conn_ops->MaxXmitDataSegmentLength);
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_PROTOCOL_ERROR, buf);
	}

	if (payload_length > conn->sess->sess_ops->FirstBurstLength) {
		pr_err("DataSegmentLength: %u is greater than"
			" FirstBurstLength: %u, protocol error.\n",
			payload_length, conn->sess->sess_ops->FirstBurstLength);
		return iscsit_add_reject_cmd(cmd,
					     ISCSI_REASON_BOOKMARK_INVALID, buf);
	}

	data_direction = (hdr->flags & ISCSI_FLAG_CMD_WRITE) ? DMA_TO_DEVICE :
			 (hdr->flags & ISCSI_FLAG_CMD_READ) ? DMA_FROM_DEVICE :
			  DMA_NONE;

	cmd->data_direction = data_direction;
	iscsi_task_attr = hdr->flags & ISCSI_FLAG_CMD_ATTR_MASK;
	/*
	 * Figure out the SAM Task Attribute for the incoming SCSI CDB
	 */
	if ((iscsi_task_attr == ISCSI_ATTR_UNTAGGED) ||
	    (iscsi_task_attr == ISCSI_ATTR_SIMPLE))
		sam_task_attr = TCM_SIMPLE_TAG;
	else if (iscsi_task_attr == ISCSI_ATTR_ORDERED)
		sam_task_attr = TCM_ORDERED_TAG;
	else if (iscsi_task_attr == ISCSI_ATTR_HEAD_OF_QUEUE)
		sam_task_attr = TCM_HEAD_TAG;
	else if (iscsi_task_attr == ISCSI_ATTR_ACA)
		sam_task_attr = TCM_ACA_TAG;
	else {
		pr_debug("Unknown iSCSI Task Attribute: 0x%02x, using"
			" TCM_SIMPLE_TAG\n", iscsi_task_attr);
		sam_task_attr = TCM_SIMPLE_TAG;
	}

	cmd->iscsi_opcode	= ISCSI_OP_SCSI_CMD;
	cmd->i_state		= ISTATE_NEW_CMD;
	cmd->immediate_cmd	= ((hdr->opcode & ISCSI_OP_IMMEDIATE) ? 1 : 0);
	cmd->immediate_data	= (payload_length) ? 1 : 0;
	cmd->unsolicited_data	= ((!(hdr->flags & ISCSI_FLAG_CMD_FINAL) &&
				     (hdr->flags & ISCSI_FLAG_CMD_WRITE)) ? 1 : 0);
	if (cmd->unsolicited_data)
		cmd->cmd_flags |= ICF_NON_IMMEDIATE_UNSOLICITED_DATA;

	conn->sess->init_task_tag = cmd->init_task_tag = hdr->itt;
	if (hdr->flags & ISCSI_FLAG_CMD_READ) {
		cmd->targ_xfer_tag = session_get_next_ttt(conn->sess);
	} else if (hdr->flags & ISCSI_FLAG_CMD_WRITE)
		cmd->targ_xfer_tag = 0xFFFFFFFF;
	cmd->cmd_sn		= be32_to_cpu(hdr->cmdsn);
	cmd->exp_stat_sn	= be32_to_cpu(hdr->exp_statsn);
	cmd->first_burst_len	= payload_length;

	if (!conn->sess->sess_ops->RDMAExtensions &&
	     cmd->data_direction == DMA_FROM_DEVICE) {
		struct iscsi_datain_req *dr;

		dr = iscsit_allocate_datain_req();
		if (!dr)
			return iscsit_add_reject_cmd(cmd,
					ISCSI_REASON_BOOKMARK_NO_RESOURCES, buf);

		iscsit_attach_datain_req(cmd, dr);
	}

	/*
	 * Initialize struct se_cmd descriptor from target_core_mod infrastructure
	 */
	transport_init_se_cmd(&cmd->se_cmd, &lio_target_fabric_configfs->tf_ops,
			conn->sess->se_sess, be32_to_cpu(hdr->data_length),
			cmd->data_direction, sam_task_attr,
			cmd->sense_buffer + 2);

	pr_debug("Got SCSI Command, ITT: 0x%08x, CmdSN: 0x%08x,"
		" ExpXferLen: %u, Length: %u, CID: %hu\n", hdr->itt,
		hdr->cmdsn, be32_to_cpu(hdr->data_length), payload_length,
		conn->cid);

	target_get_sess_cmd(conn->sess->se_sess, &cmd->se_cmd, true);

	cmd->sense_reason = transport_lookup_cmd_lun(&cmd->se_cmd,
						     scsilun_to_int(&hdr->lun));
	if (cmd->sense_reason)
		goto attach_cmd;

	cmd->sense_reason = target_setup_cmd_from_cdb(&cmd->se_cmd, hdr->cdb);
	if (cmd->sense_reason) {
		if (cmd->sense_reason == TCM_OUT_OF_RESOURCES) {
			return iscsit_add_reject_cmd(cmd,
					ISCSI_REASON_BOOKMARK_NO_RESOURCES, buf);
		}

		goto attach_cmd;
	}

	if (iscsit_build_pdu_and_seq_lists(cmd, payload_length) < 0) {
		return iscsit_add_reject_cmd(cmd,
				ISCSI_REASON_BOOKMARK_NO_RESOURCES, buf);
	}

attach_cmd:
	spin_lock_bh(&conn->cmd_lock);
	list_add_tail(&cmd->i_conn_node, &conn->conn_cmd_list);
	spin_unlock_bh(&conn->cmd_lock);
	/*
	 * Check if we need to delay processing because of ALUA
	 * Active/NonOptimized primary access state..
	 */
	core_alua_check_nonop_delay(&cmd->se_cmd);

	return 0;
}
