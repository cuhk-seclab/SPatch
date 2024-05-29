static bool get_connectable(struct hci_dev *hdev)
{
	struct pending_cmd *cmd;

	/* If there's a pending mgmt command the flag will not yet have
	 * it's final value, so check for this first.
	 */
	cmd = mgmt_pending_find(MGMT_OP_SET_CONNECTABLE, hdev);
	if (cmd) {
		struct mgmt_mode *cp = cmd->param;
		return cp->val;
	}

	return test_bit(HCI_CONNECTABLE, &hdev->dev_flags);
}
