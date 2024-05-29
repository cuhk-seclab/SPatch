static int wacom_probe(struct hid_device *hdev,
		const struct hid_device_id *id)
{
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	struct usb_device *dev = interface_to_usbdev(intf);
	struct wacom *wacom;
	struct wacom_wac *wacom_wac;
	struct wacom_features *features;
	int error;
	unsigned int connect_mask = HID_CONNECT_HIDRAW;

	if (!id->driver_data)
		return -EINVAL;

	hdev->quirks |= HID_QUIRK_NO_INIT_REPORTS;

	/* hid-core sets this quirk for the boot interface */
	hdev->quirks &= ~HID_QUIRK_NOGET;

	wacom = kzalloc(sizeof(struct wacom), GFP_KERNEL);
	if (!wacom)
		return -ENOMEM;

	hid_set_drvdata(hdev, wacom);
	wacom->hdev = hdev;

	/* ask for the report descriptor to be loaded by HID */
	error = hid_parse(hdev);
	if (error) {
		hid_err(hdev, "parse failed\n");
		goto fail_parse;
	}

	wacom_wac = &wacom->wacom_wac;
	wacom_wac->features = *((struct wacom_features *)id->driver_data);
	features = &wacom_wac->features;
	features->pktlen = wacom_compute_pktlen(hdev);
	if (features->pktlen > WACOM_PKGLEN_MAX) {
		error = -EINVAL;
		goto fail_pktlen;
	}

	if (features->check_for_hid_type && features->hid_type != hdev->type) {
		error = -ENODEV;
		goto fail_type;
	}

	wacom->usbdev = dev;
	wacom->intf = intf;
	mutex_init(&wacom->lock);
	INIT_WORK(&wacom->work, wacom_wireless_work);

	if (!(features->quirks & WACOM_QUIRK_NO_INPUT)) {
		error = wacom_allocate_inputs(wacom);
		if (error)
			goto fail_allocate_inputs;
	}

	/*
	 * Bamboo Pad has a generic hid handling for the Pen, and we switch it
	 * into debug mode for the touch part.
	 * We ignore the other interfaces.
	 */
	if (features->type == BAMBOO_PAD) {
		if (features->pktlen == WACOM_PKGLEN_PENABLED) {
			features->type = HID_GENERIC;
		} else if ((features->pktlen != WACOM_PKGLEN_BPAD_TOUCH) &&
			   (features->pktlen != WACOM_PKGLEN_BPAD_TOUCH_USB)) {
			error = -ENODEV;
			goto fail_shared_data;
		}
	}

	/* set the default size in case we do not get them from hid */
	wacom_set_default_phy(features);

	/* Retrieve the physical and logical size for touch devices */
	wacom_retrieve_hid_descriptor(hdev, features);

	/*
	 * Intuos5 has no useful data about its touch interface in its
	 * HID descriptor. If this is the touch interface (PacketSize
	 * of WACOM_PKGLEN_BBTOUCH3), override the table values.
	 */
	if (features->type >= INTUOS5S && features->type <= INTUOSHT) {
		if (features->pktlen == WACOM_PKGLEN_BBTOUCH3) {
			features->device_type = BTN_TOOL_FINGER;

			features->x_max = 4096;
			features->y_max = 4096;
		} else {
			features->device_type = BTN_TOOL_PEN;
		}
	}

	/*
	 * Same thing for Bamboo 3rd gen.
	 */
	if ((features->type == BAMBOO_PT) &&
	    (features->pktlen == WACOM_PKGLEN_BBTOUCH3) &&
	    (features->device_type == BTN_TOOL_PEN)) {
		features->device_type = BTN_TOOL_FINGER;

		features->x_max = 4096;
		features->y_max = 4096;
	}

	/*
	 * Same thing for Bamboo PAD
	 */
	if (features->type == BAMBOO_PAD)
		features->device_type = BTN_TOOL_FINGER;

	if (hdev->bus == BUS_BLUETOOTH)
		features->quirks |= WACOM_QUIRK_BATTERY;

	wacom_setup_device_quirks(features);

	/* set unit to "100th of a mm" for devices not reported by HID */
	if (!features->unit) {
		features->unit = 0x11;
		features->unitExpo = -3;
	}
	wacom_calculate_res(features);

	strlcpy(wacom_wac->name, features->name, sizeof(wacom_wac->name));
	snprintf(wacom_wac->pad_name, sizeof(wacom_wac->pad_name),
		"%s Pad", features->name);

	/* Append the device type to the name */
	if (features->device_type != BTN_TOOL_FINGER)
		strlcat(wacom_wac->name, " Pen", WACOM_NAME_MAX);
	else if (features->touch_max)
		strlcat(wacom_wac->name, " Finger", WACOM_NAME_MAX);
	else
		strlcat(wacom_wac->name, " Pad", WACOM_NAME_MAX);

	error = wacom_add_shared_data(hdev);
	if (error)
		goto fail_shared_data;

	if (!(features->quirks & WACOM_QUIRK_MONITOR) &&
	     (features->quirks & WACOM_QUIRK_BATTERY)) {
		error = wacom_initialize_battery(wacom);
		if (error)
			goto fail_battery;
	}

	if (!(features->quirks & WACOM_QUIRK_NO_INPUT)) {
		error = wacom_register_inputs(wacom);
		if (error)
			goto fail_register_inputs;
	}

	if (hdev->bus == BUS_BLUETOOTH) {
		error = device_create_file(&hdev->dev, &dev_attr_speed);
		if (error)
			hid_warn(hdev,
				 "can't create sysfs speed attribute err: %d\n",
				 error);
	}

	if (features->type == HID_GENERIC)
		connect_mask |= HID_CONNECT_DRIVER;

	/* Regular HID work starts now */
	error = hid_hw_start(hdev, connect_mask);
	if (error) {
		hid_err(hdev, "hw start failed\n");
		goto fail_hw_start;
	}

	/* Note that if query fails it is not a hard failure */
	wacom_query_tablet_data(hdev, features);

	if (features->quirks & WACOM_QUIRK_MONITOR)
		error = hid_hw_open(hdev);

	if (wacom_wac->features.type == INTUOSHT && wacom_wac->features.touch_max) {
		if (wacom_wac->features.device_type == BTN_TOOL_FINGER)
			wacom_wac->shared->touch_input = wacom_wac->input;
	}

	return 0;

fail_hw_start:
	if (hdev->bus == BUS_BLUETOOTH)
		device_remove_file(&hdev->dev, &dev_attr_speed);
fail_register_inputs:
	wacom_clean_inputs(wacom);
	wacom_destroy_battery(wacom);
fail_battery:
	wacom_remove_shared_data(wacom);
fail_shared_data:
	wacom_clean_inputs(wacom);
fail_allocate_inputs:
fail_type:
fail_pktlen:
fail_parse:
	kfree(wacom);
	hid_set_drvdata(hdev, NULL);
	return error;
}
