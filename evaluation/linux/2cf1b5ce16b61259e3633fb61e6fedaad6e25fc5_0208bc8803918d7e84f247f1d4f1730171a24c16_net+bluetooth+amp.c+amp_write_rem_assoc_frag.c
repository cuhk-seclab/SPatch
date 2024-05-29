static bool amp_write_rem_assoc_frag(struct hci_dev *hdev,
				     struct hci_conn *hcon)
{
	struct hci_cp_write_remote_amp_assoc *cp;
	struct amp_mgr *mgr = hcon->amp_mgr;
	struct amp_ctrl *ctrl;
	struct hci_request req;
	u16 frag_len, len;

	ctrl = amp_ctrl_lookup(mgr, hcon->remote_id);
	if (!ctrl)
		return false;

	if (!ctrl->assoc_rem_len) {
		BT_DBG("all fragments are written");
		ctrl->assoc_rem_len = ctrl->assoc_len;
		ctrl->assoc_len_so_far = 0;

		amp_ctrl_put(ctrl);
		return true;
	}

	frag_len = min_t(u16, 248, ctrl->assoc_rem_len);
	len = frag_len + sizeof(*cp);

	cp = kzalloc(len, GFP_KERNEL);
	if (!cp) {
		amp_ctrl_put(ctrl);
		return false;
	}

	BT_DBG("hcon %p ctrl %p frag_len %u assoc_len %u rem_len %u",
	       hcon, ctrl, frag_len, ctrl->assoc_len, ctrl->assoc_rem_len);

	cp->phy_handle = hcon->handle;
	cp->len_so_far = cpu_to_le16(ctrl->assoc_len_so_far);
	cp->rem_len = cpu_to_le16(ctrl->assoc_rem_len);
	memcpy(cp->frag, ctrl->assoc, frag_len);

	ctrl->assoc_len_so_far += frag_len;
	ctrl->assoc_rem_len -= frag_len;

	amp_ctrl_put(ctrl);

	hci_req_init(&req, hdev);
	hci_req_add(&req, HCI_OP_WRITE_REMOTE_AMP_ASSOC, len, cp);
	hci_req_run_skb(&req, write_remote_amp_assoc_complete);

	kfree(cp);

	return false;
}
