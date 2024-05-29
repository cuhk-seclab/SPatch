static int usbvision_probe(struct usb_interface *intf,
			   const struct usb_device_id *devid)
{
	struct usb_device *dev = usb_get_dev(interface_to_usbdev(intf));
	struct usb_interface *uif;
	__u8 ifnum = intf->altsetting->desc.bInterfaceNumber;
	const struct usb_host_interface *interface;
	struct usb_usbvision *usbvision = NULL;
	const struct usb_endpoint_descriptor *endpoint;
	int model, i, ret;

	PDEBUG(DBG_PROBE, "VID=%#04x, PID=%#04x, ifnum=%u",
				dev->descriptor.idVendor,
				dev->descriptor.idProduct, ifnum);

	model = devid->driver_info;
	if (model < 0 || model >= usbvision_device_data_size) {
		PDEBUG(DBG_PROBE, "model out of bounds %d", model);
		ret = -ENODEV;
		goto err_usb;
	}
	printk(KERN_INFO "%s: %s found\n", __func__,
				usbvision_device_data[model].model_string);

	/*
	 * this is a security check.
	 * an exploit using an incorrect bInterfaceNumber is known
	 */
	if (ifnum >= USB_MAXINTERFACES || !dev->actconfig->interface[ifnum])
		return -ENODEV;

	if (usbvision_device_data[model].interface >= 0)
		interface = &dev->actconfig->interface[usbvision_device_data[model].interface]->altsetting[0];
	else if (ifnum < dev->actconfig->desc.bNumInterfaces)
		interface = &dev->actconfig->interface[ifnum]->altsetting[0];
	else {
		dev_err(&intf->dev, "interface %d is invalid, max is %d\n",
		    ifnum, dev->actconfig->desc.bNumInterfaces - 1);
		ret = -ENODEV;
		goto err_usb;
	}

	if (interface->desc.bNumEndpoints < 2) {
		dev_err(&intf->dev, "interface %d has %d endpoints, but must"
		    " have minimum 2\n", ifnum, interface->desc.bNumEndpoints);
		ret = -ENODEV;
		goto err_usb;
	}
	endpoint = &interface->endpoint[1].desc;

	if (!usb_endpoint_xfer_isoc(endpoint)) {
		dev_err(&intf->dev, "%s: interface %d. has non-ISO endpoint!\n",
		    __func__, ifnum);
		dev_err(&intf->dev, "%s: Endpoint attributes %d",
		    __func__, endpoint->bmAttributes);
		ret = -ENODEV;
		goto err_usb;
	}
	if (usb_endpoint_dir_out(endpoint)) {
		dev_err(&intf->dev, "%s: interface %d. has ISO OUT endpoint!\n",
		    __func__, ifnum);
		ret = -ENODEV;
		goto err_usb;
	}

	usbvision = usbvision_alloc(dev, intf);
	if (usbvision == NULL) {
		dev_err(&intf->dev, "%s: couldn't allocate USBVision struct\n", __func__);
		ret = -ENOMEM;
		goto err_usb;
	}

	if (dev->descriptor.bNumConfigurations > 1)
		usbvision->bridge_type = BRIDGE_NT1004;
	else if (model == DAZZLE_DVC_90_REV_1_SECAM)
		usbvision->bridge_type = BRIDGE_NT1005;
	else
		usbvision->bridge_type = BRIDGE_NT1003;
	PDEBUG(DBG_PROBE, "bridge_type %d", usbvision->bridge_type);

	/* compute alternate max packet sizes */
	uif = dev->actconfig->interface[0];

	usbvision->num_alt = uif->num_altsetting;
	PDEBUG(DBG_PROBE, "Alternate settings: %i", usbvision->num_alt);
	usbvision->alt_max_pkt_size = kmalloc(32 * usbvision->num_alt, GFP_KERNEL);
	if (usbvision->alt_max_pkt_size == NULL) {
		dev_err(&intf->dev, "usbvision: out of memory!\n");
		ret = -ENOMEM;
		goto err_pkt;
	}

	for (i = 0; i < usbvision->num_alt; i++) {
		u16 tmp = le16_to_cpu(uif->altsetting[i].endpoint[1].desc.
				      wMaxPacketSize);
		usbvision->alt_max_pkt_size[i] =
			(tmp & 0x07ff) * (((tmp & 0x1800) >> 11) + 1);
		PDEBUG(DBG_PROBE, "Alternate setting %i, max size= %i", i,
		       usbvision->alt_max_pkt_size[i]);
	}


	usbvision->nr = usbvision_nr++;

	spin_lock_init(&usbvision->queue_lock);
	init_waitqueue_head(&usbvision->wait_frame);
	init_waitqueue_head(&usbvision->wait_stream);

	usbvision->have_tuner = usbvision_device_data[model].tuner;
	if (usbvision->have_tuner)
		usbvision->tuner_type = usbvision_device_data[model].tuner_type;

	usbvision->dev_model = model;
	usbvision->remove_pending = 0;
	usbvision->iface = ifnum;
	usbvision->iface_alt = 0;
	usbvision->video_endp = endpoint->bEndpointAddress;
	usbvision->isoc_packet_size = 0;
	usbvision->usb_bandwidth = 0;
	usbvision->user = 0;
	usbvision->streaming = stream_off;
	usbvision_configure_video(usbvision);
	usbvision_register_video(usbvision);

	usbvision_create_sysfs(&usbvision->vdev);

	PDEBUG(DBG_PROBE, "success");
	return 0;

err_pkt:
	usbvision_release(usbvision);
err_usb:
	usb_put_dev(dev);
	return ret;
}
