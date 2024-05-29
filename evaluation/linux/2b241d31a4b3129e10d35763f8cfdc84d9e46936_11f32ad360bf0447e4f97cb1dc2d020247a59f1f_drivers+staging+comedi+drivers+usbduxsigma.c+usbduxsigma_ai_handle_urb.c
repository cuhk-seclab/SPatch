static void usbduxsigma_ai_handle_urb(struct comedi_device *dev,
				      struct comedi_subdevice *s,
				      struct urb *urb)
{
	struct usbduxsigma_private *devpriv = dev->private;
	struct comedi_async *async = s->async;
	struct comedi_cmd *cmd = &async->cmd;
	uint32_t val;
	int ret;
	int i;

	if ((urb->actual_length > 0) && (urb->status != -EXDEV)) {
		devpriv->ai_counter--;
		if (devpriv->ai_counter == 0) {
			devpriv->ai_counter = devpriv->ai_timer;

			/* get the data from the USB bus
			   and hand it over to comedi */
			for (i = 0; i < cmd->chanlist_len; i++) {
				/* transfer data,
				   note first byte is the DIO state */
				val = be32_to_cpu(devpriv->in_buf[i+1]);
				val &= 0x00ffffff; /* strip status byte */
				val ^= 0x00800000; /* convert to unsigned */

				if (!comedi_buf_write_samples(s, &val, 1))
					return;
			}

			if (cmd->stop_src == TRIG_COUNT &&
			    async->scans_done >= cmd->stop_arg)
				async->events |= COMEDI_CB_EOA;
		}
	}

	/* if command is still running, resubmit urb */
	if (!(async->events & COMEDI_CB_CANCEL_MASK)) {
		urb->dev = comedi_to_usb_dev(dev);
		ret = usb_submit_urb(urb, GFP_ATOMIC);
		if (ret < 0) {
			dev_err(dev->class_dev, "urb resubmit failed (%d)\n",
				ret);
			if (ret == -EL2NSYNC)
				dev_err(dev->class_dev,
					"buggy USB host controller or bug in IRQ handler\n");
			async->events |= COMEDI_CB_ERROR;
		}
	}
}
