static int add_advertising(struct sock *sk, struct hci_dev *hdev,
			   void *data, u16 data_len)
{
	struct mgmt_cp_add_advertising *cp = data;
	struct mgmt_rp_add_advertising rp;
	u32 flags;
	u32 supported_flags;
	u8 status;
	u16 timeout, duration;
	unsigned int prev_instance_cnt = hdev->adv_instance_cnt;
	u8 schedule_instance = 0;
	struct adv_info *next_instance;
	int err;
	struct mgmt_pending_cmd *cmd;
	struct hci_request req;

	BT_DBG("%s", hdev->name);

	status = mgmt_le_support(hdev);
	if (status)
		return mgmt_cmd_status(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
				       status);

	if (cp->instance < 1 || cp->instance > HCI_MAX_ADV_INSTANCES)
		return mgmt_cmd_status(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
				       MGMT_STATUS_INVALID_PARAMS);

	flags = __le32_to_cpu(cp->flags);
	timeout = __le16_to_cpu(cp->timeout);
	duration = __le16_to_cpu(cp->duration);

	/* The current implementation only supports a subset of the specified
	 * flags.
	 */
	supported_flags = get_supported_adv_flags(hdev);
	if (flags & ~supported_flags)
		return mgmt_cmd_status(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
				       MGMT_STATUS_INVALID_PARAMS);

	hci_dev_lock(hdev);

	if (timeout && !hdev_is_powered(hdev)) {
		err = mgmt_cmd_status(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
				      MGMT_STATUS_REJECTED);
		goto unlock;
	}

	if (pending_find(MGMT_OP_ADD_ADVERTISING, hdev) ||
	    pending_find(MGMT_OP_REMOVE_ADVERTISING, hdev) ||
	    pending_find(MGMT_OP_SET_LE, hdev)) {
		err = mgmt_cmd_status(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
				      MGMT_STATUS_BUSY);
		goto unlock;
	}

	if (!tlv_data_is_valid(hdev, flags, cp->data, cp->adv_data_len, true) ||
	    !tlv_data_is_valid(hdev, flags, cp->data + cp->adv_data_len,
			       cp->scan_rsp_len, false)) {
		err = mgmt_cmd_status(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
				      MGMT_STATUS_INVALID_PARAMS);
		goto unlock;
	}

	err = hci_add_adv_instance(hdev, cp->instance, flags,
				   cp->adv_data_len, cp->data,
				   cp->scan_rsp_len,
				   cp->data + cp->adv_data_len,
				   timeout, duration);
	if (err < 0) {
		err = mgmt_cmd_status(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
				      MGMT_STATUS_FAILED);
		goto unlock;
	}

	/* Only trigger an advertising added event if a new instance was
	 * actually added.
	 */
	if (hdev->adv_instance_cnt > prev_instance_cnt)
		mgmt_advertising_added(sk, hdev, cp->instance);

	if (hdev->cur_adv_instance == cp->instance) {
		/* If the currently advertised instance is being changed then
		 * cancel the current advertising and schedule the next
		 * instance. If there is only one instance then the overridden
		 * advertising data will be visible right away.
		 */
		cancel_adv_timeout(hdev);

		next_instance = hci_get_next_instance(hdev, cp->instance);
		if (next_instance)
			schedule_instance = next_instance->instance;
	} else if (!hdev->adv_instance_timeout) {
		/* Immediately advertise the new instance if no other
		 * instance is currently being advertised.
		 */
		schedule_instance = cp->instance;
	}

	/* If the HCI_ADVERTISING flag is set or the device isn't powered or
	 * there is no instance to be advertised then we have no HCI
	 * communication to make. Simply return.
	 */
	if (!hdev_is_powered(hdev) ||
	    hci_dev_test_flag(hdev, HCI_ADVERTISING) ||
	    !schedule_instance) {
		rp.instance = cp->instance;
		err = mgmt_cmd_complete(sk, hdev->id, MGMT_OP_ADD_ADVERTISING,
					MGMT_STATUS_SUCCESS, &rp, sizeof(rp));
		goto unlock;
	}

	/* We're good to go, update advertising data, parameters, and start
	 * advertising.
	 */
	cmd = mgmt_pending_add(sk, MGMT_OP_ADD_ADVERTISING, hdev, data,
			       data_len);
	if (!cmd) {
		err = -ENOMEM;
		goto unlock;
	}

	hci_req_init(&req, hdev);

	err = __hci_req_schedule_adv_instance(&req, schedule_instance, true);

	if (!err)
		err = hci_req_run(&req, add_advertising_complete);

	if (err < 0)
		mgmt_pending_remove(cmd);

unlock:
	hci_dev_unlock(hdev);

	return err;
}
