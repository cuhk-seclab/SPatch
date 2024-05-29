static void cx231xx_usb_disconnect(struct usb_interface *interface)
{
	struct cx231xx *dev;

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	if (!dev)
		return;

	if (!dev->udev)
		return;

	dev->state |= DEV_DISCONNECTED;

	flush_request_modules(dev);

	/* wait until all current v4l2 io is finished then deallocate
	   resources */
	mutex_lock(&dev->lock);

	wake_up_interruptible_all(&dev->open);

	if (dev->users) {
		dev_warn(dev->dev,
			 "device %s is open! Deregistration and memory deallocation are deferred on close.\n",
			 video_device_node_name(&dev->vdev));

		/* Even having users, it is safe to remove the RC i2c driver */
		cx231xx_ir_exit(dev);

		if (dev->USE_ISO)
			cx231xx_uninit_isoc(dev);
		else
			cx231xx_uninit_bulk(dev);
		wake_up_interruptible(&dev->wait_frame);
		wake_up_interruptible(&dev->wait_stream);
	} else {
	}

	cx231xx_close_extension(dev);

	mutex_unlock(&dev->lock);

	if (!dev->users)
		cx231xx_release_resources(dev);
}
