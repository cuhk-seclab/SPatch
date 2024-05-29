int mei_hbm_dispatch(struct mei_device *dev, struct mei_msg_hdr *hdr)
{
	struct mei_bus_message *mei_msg;
	struct hbm_host_version_response *version_res;
	struct hbm_props_response *props_res;
	struct hbm_host_enum_response *enum_res;
	struct hbm_add_client_request *add_cl_req;
	int ret;

	struct mei_hbm_cl_cmd *cl_cmd;
	struct hbm_client_connect_request *disconnect_req;
	struct hbm_flow_control *flow_control;

	/* read the message to our buffer */
	BUG_ON(hdr->length >= sizeof(dev->rd_msg_buf));
	mei_read_slots(dev, dev->rd_msg_buf, hdr->length);
	mei_msg = (struct mei_bus_message *)dev->rd_msg_buf;
	cl_cmd  = (struct mei_hbm_cl_cmd *)mei_msg;

	/* ignore spurious message and prevent reset nesting
	 * hbm is put to idle during system reset
	 */
	if (dev->hbm_state == MEI_HBM_IDLE) {
		dev_dbg(dev->dev, "hbm: state is idle ignore spurious messages\n");
		return 0;
	}

	switch (mei_msg->hbm_cmd) {
	case HOST_START_RES_CMD:
		dev_dbg(dev->dev, "hbm: start: response message received.\n");

		dev->init_clients_timer = 0;

		version_res = (struct hbm_host_version_response *)mei_msg;

		dev_dbg(dev->dev, "HBM VERSION: DRIVER=%02d:%02d DEVICE=%02d:%02d\n",
				HBM_MAJOR_VERSION, HBM_MINOR_VERSION,
				version_res->me_max_version.major_version,
				version_res->me_max_version.minor_version);

		if (version_res->host_version_supported) {
			dev->version.major_version = HBM_MAJOR_VERSION;
			dev->version.minor_version = HBM_MINOR_VERSION;
		} else {
			dev->version.major_version =
				version_res->me_max_version.major_version;
			dev->version.minor_version =
				version_res->me_max_version.minor_version;
		}

		if (!mei_hbm_version_is_supported(dev)) {
			dev_warn(dev->dev, "hbm: start: version mismatch - stopping the driver.\n");

			dev->hbm_state = MEI_HBM_STOPPED;
			if (mei_hbm_stop_req(dev)) {
				dev_err(dev->dev, "hbm: start: failed to send stop request\n");
				return -EIO;
			}
			break;
		}

		mei_hbm_config_features(dev);

		if (dev->dev_state != MEI_DEV_INIT_CLIENTS ||
		    dev->hbm_state != MEI_HBM_STARTING) {
			dev_err(dev->dev, "hbm: start: state mismatch, [%d, %d]\n",
				dev->dev_state, dev->hbm_state);
			return -EPROTO;
		}

		if (mei_hbm_enum_clients_req(dev)) {
			dev_err(dev->dev, "hbm: start: failed to send enumeration request\n");
			return -EIO;
		}

		wake_up(&dev->wait_hbm_start);
		break;

	case CLIENT_CONNECT_RES_CMD:
		dev_dbg(dev->dev, "hbm: client connect response: message received.\n");
		mei_hbm_cl_res(dev, cl_cmd, MEI_FOP_CONNECT);
		break;

	case CLIENT_DISCONNECT_RES_CMD:
		dev_dbg(dev->dev, "hbm: client disconnect response: message received.\n");
		mei_hbm_cl_res(dev, cl_cmd, MEI_FOP_DISCONNECT);
		break;

	case MEI_FLOW_CONTROL_CMD:
		dev_dbg(dev->dev, "hbm: client flow control response: message received.\n");

		flow_control = (struct hbm_flow_control *) mei_msg;
		mei_hbm_cl_flow_control_res(dev, flow_control);
		break;

	case MEI_PG_ISOLATION_ENTRY_RES_CMD:
		dev_dbg(dev->dev, "hbm: power gate isolation entry response received\n");
		ret = mei_hbm_pg_enter_res(dev);
		if (ret)
			return ret;
		break;

	case MEI_PG_ISOLATION_EXIT_REQ_CMD:
		dev_dbg(dev->dev, "hbm: power gate isolation exit request received\n");
		ret = mei_hbm_pg_exit_res(dev);
		if (ret)
			return ret;
		break;

	case HOST_CLIENT_PROPERTIES_RES_CMD:
		dev_dbg(dev->dev, "hbm: properties response: message received.\n");

		dev->init_clients_timer = 0;

		if (dev->dev_state != MEI_DEV_INIT_CLIENTS ||
		    dev->hbm_state != MEI_HBM_CLIENT_PROPERTIES) {
			dev_err(dev->dev, "hbm: properties response: state mismatch, [%d, %d]\n",
				dev->dev_state, dev->hbm_state);
			return -EPROTO;
		}

		props_res = (struct hbm_props_response *)mei_msg;

		if (props_res->status) {
			dev_err(dev->dev, "hbm: properties response: wrong status = %d %s\n",
				props_res->status,
				mei_hbm_status_str(props_res->status));
			return -EPROTO;
		}

		mei_hbm_me_cl_add(dev, props_res);

		dev->me_client_index++;

		/* request property for the next client */
		if (mei_hbm_prop_req(dev))
			return -EIO;

		break;

	case HOST_ENUM_RES_CMD:
		dev_dbg(dev->dev, "hbm: enumeration response: message received\n");

		dev->init_clients_timer = 0;

		enum_res = (struct hbm_host_enum_response *) mei_msg;
		BUILD_BUG_ON(sizeof(dev->me_clients_map)
				< sizeof(enum_res->valid_addresses));
		memcpy(dev->me_clients_map, enum_res->valid_addresses,
				sizeof(enum_res->valid_addresses));

		if (dev->dev_state != MEI_DEV_INIT_CLIENTS ||
		    dev->hbm_state != MEI_HBM_ENUM_CLIENTS) {
			dev_err(dev->dev, "hbm: enumeration response: state mismatch, [%d, %d]\n",
				dev->dev_state, dev->hbm_state);
			return -EPROTO;
		}

		dev->hbm_state = MEI_HBM_CLIENT_PROPERTIES;

		/* first property request */
		if (mei_hbm_prop_req(dev))
			return -EIO;

		break;

	case HOST_STOP_RES_CMD:
		dev_dbg(dev->dev, "hbm: stop response: message received\n");

		dev->init_clients_timer = 0;

		if (dev->hbm_state != MEI_HBM_STOPPED) {
			dev_err(dev->dev, "hbm: stop response: state mismatch, [%d, %d]\n",
				dev->dev_state, dev->hbm_state);
			return -EPROTO;
		}

		dev->dev_state = MEI_DEV_POWER_DOWN;
		dev_info(dev->dev, "hbm: stop response: resetting.\n");
		/* force the reset */
		return -EPROTO;
		break;

	case CLIENT_DISCONNECT_REQ_CMD:
		dev_dbg(dev->dev, "hbm: disconnect request: message received\n");

		disconnect_req = (struct hbm_client_connect_request *)mei_msg;
		mei_hbm_fw_disconnect_req(dev, disconnect_req);
		break;

	case ME_STOP_REQ_CMD:
		dev_dbg(dev->dev, "hbm: stop request: message received\n");
		dev->hbm_state = MEI_HBM_STOPPED;
		if (mei_hbm_stop_req(dev)) {
			dev_err(dev->dev, "hbm: stop request: failed to send stop request\n");
			return -EIO;
		}
		break;

	case MEI_HBM_ADD_CLIENT_REQ_CMD:
		dev_dbg(dev->dev, "hbm: add client request received\n");
		/*
		 * after the host receives the enum_resp
		 * message clients may be added or removed
		 */
		if (dev->hbm_state <= MEI_HBM_ENUM_CLIENTS &&
		    dev->hbm_state >= MEI_HBM_STOPPED) {
			dev_err(dev->dev, "hbm: add client: state mismatch, [%d, %d]\n",
				dev->dev_state, dev->hbm_state);
			return -EPROTO;
		}
		add_cl_req = (struct hbm_add_client_request *)mei_msg;
		ret = mei_hbm_fw_add_cl_req(dev, add_cl_req);
		if (ret) {
			dev_err(dev->dev, "hbm: add client: failed to send response %d\n",
				ret);
			return -EIO;
		}
		dev_dbg(dev->dev, "hbm: add client request processed\n");
		break;

	case MEI_HBM_NOTIFY_RES_CMD:
		dev_dbg(dev->dev, "hbm: notify response received\n");
		mei_hbm_cl_res(dev, cl_cmd, notify_res_to_fop(cl_cmd));
		break;

	case MEI_HBM_NOTIFICATION_CMD:
		dev_dbg(dev->dev, "hbm: notification\n");
		mei_hbm_cl_notify(dev, cl_cmd);
		break;

	default:
		BUG();
		break;

	}
	return 0;
}
