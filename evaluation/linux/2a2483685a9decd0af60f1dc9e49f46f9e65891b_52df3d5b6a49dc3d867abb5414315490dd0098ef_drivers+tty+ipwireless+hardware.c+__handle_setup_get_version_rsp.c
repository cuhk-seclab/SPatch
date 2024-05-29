static void __handle_setup_get_version_rsp(struct ipw_hardware *hw)
{
	struct ipw_setup_config_packet *config_packet;
	struct ipw_setup_config_done_packet *config_done_packet;
	struct ipw_setup_open_packet *open_packet;
	struct ipw_setup_info_packet *info_packet;
	int port;
	unsigned int channel_idx;

	/* generate config packet */
	for (port = 1; port <= NL_NUM_OF_ADDRESSES; port++) {
		config_packet = alloc_ctrl_packet(
				sizeof(struct ipw_setup_config_packet),
				ADDR_SETUP_PROT,
				TL_PROTOCOLID_SETUP,
				TL_SETUP_SIGNO_CONFIG_MSG);
		if (!config_packet)
			goto exit_nomem;
		config_packet->header.length = sizeof(struct tl_setup_config_msg);
		config_packet->body.port_no = port;
		config_packet->body.prio_data = PRIO_DATA;
		config_packet->body.prio_ctrl = PRIO_CTRL;
		send_packet(hw, PRIO_SETUP, &config_packet->header);
	}
	config_done_packet = alloc_ctrl_packet(
			sizeof(struct ipw_setup_config_done_packet),
			ADDR_SETUP_PROT,
			TL_PROTOCOLID_SETUP,
			TL_SETUP_SIGNO_CONFIG_DONE_MSG);
	if (!config_done_packet)
		goto exit_nomem;
	config_done_packet->header.length = sizeof(struct tl_setup_config_done_msg);
	send_packet(hw, PRIO_SETUP, &config_done_packet->header);

	/* generate open packet */
	for (port = 1; port <= NL_NUM_OF_ADDRESSES; port++) {
		open_packet = alloc_ctrl_packet(
				sizeof(struct ipw_setup_open_packet),
				ADDR_SETUP_PROT,
				TL_PROTOCOLID_SETUP,
				TL_SETUP_SIGNO_OPEN_MSG);
		if (!open_packet)
			goto exit_nomem;
		open_packet->header.length = sizeof(struct tl_setup_open_msg);
		open_packet->body.port_no = port;
		send_packet(hw, PRIO_SETUP, &open_packet->header);
	}
	for (channel_idx = 0;
			channel_idx < NL_NUM_OF_ADDRESSES; channel_idx++) {
		int ret;

		ret = set_DTR(hw, PRIO_SETUP, channel_idx,
			(hw->control_lines[channel_idx] &
			 IPW_CONTROL_LINE_DTR) != 0);
		if (ret) {
			printk(KERN_ERR IPWIRELESS_PCCARD_NAME
					": error setting DTR (%d)\n", ret);
			return;
		}

		set_RTS(hw, PRIO_SETUP, channel_idx,
			(hw->control_lines [channel_idx] &
			 IPW_CONTROL_LINE_RTS) != 0);
		if (ret) {
			printk(KERN_ERR IPWIRELESS_PCCARD_NAME
					": error setting RTS (%d)\n", ret);
			return;
		}
	}
	/*
	 * For NDIS we assume that we are using sync PPP frames, for COM async.
	 * This driver uses NDIS mode too. We don't bother with translation
	 * from async -> sync PPP.
	 */
	info_packet = alloc_ctrl_packet(sizeof(struct ipw_setup_info_packet),
			ADDR_SETUP_PROT,
			TL_PROTOCOLID_SETUP,
			TL_SETUP_SIGNO_INFO_MSG);
	if (!info_packet)
		goto exit_nomem;
	info_packet->header.length = sizeof(struct tl_setup_info_msg);
	info_packet->body.driver_type = NDISWAN_DRIVER;
	info_packet->body.major_version = NDISWAN_DRIVER_MAJOR_VERSION;
	info_packet->body.minor_version = NDISWAN_DRIVER_MINOR_VERSION;
	send_packet(hw, PRIO_SETUP, &info_packet->header);

	/* Initialization is now complete, so we clear the 'to_setup' flag */
	hw->to_setup = 0;

	return;

exit_nomem:
	printk(KERN_ERR IPWIRELESS_PCCARD_NAME
			": not enough memory to alloc control packet\n");
	hw->to_setup = -1;
}
