static void srp_handle_qp_err(u64 wr_id, enum ib_wc_status wc_status,
			      bool send_err, struct srp_rdma_ch *ch)
{
	struct srp_target_port *target = ch->target;

	if (wr_id == SRP_LAST_WR_ID) {
		complete(&ch->done);
		return;
	}

	if (target->connected && !target->qp_in_error) {
		if (wr_id & LOCAL_INV_WR_ID_MASK) {
			shost_printk(KERN_ERR, target->scsi_host, PFX
				     "LOCAL_INV failed with status %s (%d)\n",
				     ib_wc_status_msg(wc_status), wc_status);
		} else if (wr_id & FAST_REG_WR_ID_MASK) {
			shost_printk(KERN_ERR, target->scsi_host, PFX
				     "FAST_REG_MR failed status %s (%d)\n",
				     ib_wc_status_msg(wc_status), wc_status);
		} else {
			shost_printk(KERN_ERR, target->scsi_host,
				     PFX "failed %s status %s (%d) for iu %p\n",
				     send_err ? "send" : "receive",
				     ib_wc_status_msg(wc_status), wc_status,
				     (void *)(uintptr_t)wr_id);
		}
		queue_work(system_long_wq, &target->tl_err_work);
	}
	target->qp_in_error = true;
}
