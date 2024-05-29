static void
qlafx00_status_entry(scsi_qla_host_t *vha, struct rsp_que *rsp, void *pkt)
{
	srb_t		*sp;
	fc_port_t	*fcport;
	struct scsi_cmnd *cp;
	struct sts_entry_fx00 *sts;
	__le16		comp_status;
	__le16		scsi_status;
	uint16_t	ox_id;
	__le16		lscsi_status;
	int32_t		resid;
	uint32_t	sense_len, par_sense_len, rsp_info_len, resid_len,
	    fw_resid_len;
	uint8_t		*rsp_info = NULL, *sense_data = NULL;
	struct qla_hw_data *ha = vha->hw;
	uint32_t hindex, handle;
	uint16_t que;
	struct req_que *req;
	int logit = 1;
	int res = 0;

	sts = (struct sts_entry_fx00 *) pkt;

	comp_status = sts->comp_status;
	scsi_status = sts->scsi_status & cpu_to_le16((uint16_t)SS_MASK);
	hindex = sts->handle;
	handle = LSW(hindex);

	que = MSW(hindex);
	req = ha->req_q_map[que];

	/* Validate handle. */
	if (handle < req->num_outstanding_cmds)
		sp = req->outstanding_cmds[handle];
	else
		sp = NULL;

	if (sp == NULL) {
		ql_dbg(ql_dbg_io, vha, 0x3034,
		    "Invalid status handle (0x%x).\n", handle);

		set_bit(ISP_ABORT_NEEDED, &vha->dpc_flags);
		qla2xxx_wake_dpc(vha);
		return;
	}

	if (sp->type == SRB_TM_CMD) {
		req->outstanding_cmds[handle] = NULL;
		qlafx00_tm_iocb_entry(vha, req, pkt, sp,
		    scsi_status, comp_status);
		return;
	}

	/* Fast path completion. */
	if (comp_status == CS_COMPLETE && scsi_status == 0) {
		qla2x00_process_completed_request(vha, req, handle);
		return;
	}

	req->outstanding_cmds[handle] = NULL;
	cp = GET_CMD_SP(sp);
	if (cp == NULL) {
		ql_dbg(ql_dbg_io, vha, 0x3048,
		    "Command already returned (0x%x/%p).\n",
		    handle, sp);

		return;
	}

	lscsi_status = scsi_status & cpu_to_le16((uint16_t)STATUS_MASK);

	fcport = sp->fcport;

	ox_id = 0;
	sense_len = par_sense_len = rsp_info_len = resid_len =
		fw_resid_len = 0;
	if (scsi_status & cpu_to_le16((uint16_t)SS_SENSE_LEN_VALID))
		sense_len = sts->sense_len;
	if (scsi_status & cpu_to_le16(((uint16_t)SS_RESIDUAL_UNDER
	    | (uint16_t)SS_RESIDUAL_OVER)))
		resid_len = le32_to_cpu(sts->residual_len);
	if (comp_status == cpu_to_le16((uint16_t)CS_DATA_UNDERRUN))
		fw_resid_len = le32_to_cpu(sts->residual_len);
	rsp_info = sense_data = sts->data;
	par_sense_len = sizeof(sts->data);

	/* Check for overrun. */
	if (comp_status == CS_COMPLETE &&
	    scsi_status & cpu_to_le16((uint16_t)SS_RESIDUAL_OVER))
		comp_status = cpu_to_le16((uint16_t)CS_DATA_OVERRUN);

	/*
	 * Based on Host and scsi status generate status code for Linux
	 */
	switch (le16_to_cpu(comp_status)) {
	case CS_COMPLETE:
	case CS_QUEUE_FULL:
		if (scsi_status == 0) {
			res = DID_OK << 16;
			break;
		}
		if (scsi_status & cpu_to_le16(((uint16_t)SS_RESIDUAL_UNDER
		    | (uint16_t)SS_RESIDUAL_OVER))) {
			resid = resid_len;
			scsi_set_resid(cp, resid);

			if (!lscsi_status &&
			    ((unsigned)(scsi_bufflen(cp) - resid) <
			     cp->underflow)) {
				ql_dbg(ql_dbg_io, fcport->vha, 0x3050,
				    "Mid-layer underflow "
				    "detected (0x%x of 0x%x bytes).\n",
				    resid, scsi_bufflen(cp));

				res = DID_ERROR << 16;
				break;
			}
		}
		res = DID_OK << 16 | le16_to_cpu(lscsi_status);

		if (lscsi_status ==
		    cpu_to_le16((uint16_t)SAM_STAT_TASK_SET_FULL)) {
			ql_dbg(ql_dbg_io, fcport->vha, 0x3051,
			    "QUEUE FULL detected.\n");
			break;
		}
		logit = 0;
		if (lscsi_status != cpu_to_le16((uint16_t)SS_CHECK_CONDITION))
			break;

		memset(cp->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
		if (!(scsi_status & cpu_to_le16((uint16_t)SS_SENSE_LEN_VALID)))
			break;

		qlafx00_handle_sense(sp, sense_data, par_sense_len, sense_len,
		    rsp, res);
		break;

	case CS_DATA_UNDERRUN:
		/* Use F/W calculated residual length. */
		if (IS_FWI2_CAPABLE(ha) || IS_QLAFX00(ha))
			resid = fw_resid_len;
		else
			resid = resid_len;
		scsi_set_resid(cp, resid);
		if (scsi_status & cpu_to_le16((uint16_t)SS_RESIDUAL_UNDER)) {
			if ((IS_FWI2_CAPABLE(ha) || IS_QLAFX00(ha))
			    && fw_resid_len != resid_len) {
				ql_dbg(ql_dbg_io, fcport->vha, 0x3052,
				    "Dropped frame(s) detected "
				    "(0x%x of 0x%x bytes).\n",
				    resid, scsi_bufflen(cp));

				res = DID_ERROR << 16 |
				    le16_to_cpu(lscsi_status);
				goto check_scsi_status;
			}

			if (!lscsi_status &&
			    ((unsigned)(scsi_bufflen(cp) - resid) <
			    cp->underflow)) {
				ql_dbg(ql_dbg_io, fcport->vha, 0x3053,
				    "Mid-layer underflow "
				    "detected (0x%x of 0x%x bytes, "
				    "cp->underflow: 0x%x).\n",
				    resid, scsi_bufflen(cp), cp->underflow);

				res = DID_ERROR << 16;
				break;
			}
		} else if (lscsi_status !=
		    cpu_to_le16((uint16_t)SAM_STAT_TASK_SET_FULL) &&
		    lscsi_status != cpu_to_le16((uint16_t)SAM_STAT_BUSY)) {
			/*
			 * scsi status of task set and busy are considered
			 * to be task not completed.
			 */

			ql_dbg(ql_dbg_io, fcport->vha, 0x3054,
			    "Dropped frame(s) detected (0x%x "
			    "of 0x%x bytes).\n", resid,
			    scsi_bufflen(cp));

			res = DID_ERROR << 16 | le16_to_cpu(lscsi_status);
			goto check_scsi_status;
		} else {
			ql_dbg(ql_dbg_io, fcport->vha, 0x3055,
			    "scsi_status: 0x%x, lscsi_status: 0x%x\n",
			    scsi_status, lscsi_status);
		}

		res = DID_OK << 16 | le16_to_cpu(lscsi_status);
		logit = 0;

check_scsi_status:
		/*
		 * Check to see if SCSI Status is non zero. If so report SCSI
		 * Status.
		 */
		if (lscsi_status != 0) {
			if (lscsi_status ==
			    cpu_to_le16((uint16_t)SAM_STAT_TASK_SET_FULL)) {
				ql_dbg(ql_dbg_io, fcport->vha, 0x3056,
				    "QUEUE FULL detected.\n");
				logit = 1;
				break;
			}
			if (lscsi_status !=
			    cpu_to_le16((uint16_t)SS_CHECK_CONDITION))
				break;

			memset(cp->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
			if (!(scsi_status &
			    cpu_to_le16((uint16_t)SS_SENSE_LEN_VALID)))
				break;

			qlafx00_handle_sense(sp, sense_data, par_sense_len,
			    sense_len, rsp, res);
		}
		break;

	case CS_PORT_LOGGED_OUT:
	case CS_PORT_CONFIG_CHG:
	case CS_PORT_BUSY:
	case CS_INCOMPLETE:
	case CS_PORT_UNAVAILABLE:
	case CS_TIMEOUT:
	case CS_RESET:

		/*
		 * We are going to have the fc class block the rport
		 * while we try to recover so instruct the mid layer
		 * to requeue until the class decides how to handle this.
		 */
		res = DID_TRANSPORT_DISRUPTED << 16;

		ql_dbg(ql_dbg_io, fcport->vha, 0x3057,
		    "Port down status: port-state=0x%x.\n",
		    atomic_read(&fcport->state));

		if (atomic_read(&fcport->state) == FCS_ONLINE)
			qla2x00_mark_device_lost(fcport->vha, fcport, 1, 1);
		break;

	case CS_ABORTED:
		res = DID_RESET << 16;
		break;

	default:
		res = DID_ERROR << 16;
		break;
	}

	if (logit)
		ql_dbg(ql_dbg_io, fcport->vha, 0x3058,
		    "FCP command status: 0x%x-0x%x (0x%x) nexus=%ld:%d:%llu "
		    "tgt_id: 0x%x lscsi_status: 0x%x cdb=%10phN len=0x%x "
		    "rsp_info=%p resid=0x%x fw_resid=0x%x sense_len=0x%x, "
		    "par_sense_len=0x%x, rsp_info_len=0x%x\n",
		    comp_status, scsi_status, res, vha->host_no,
		    cp->device->id, cp->device->lun, fcport->tgt_id,
		    lscsi_status, cp->cmnd, scsi_bufflen(cp),
		    rsp_info, resid_len, fw_resid_len, sense_len,
		    par_sense_len, rsp_info_len);

	if (rsp->status_srb == NULL)
		sp->done(ha, sp, res);
}
