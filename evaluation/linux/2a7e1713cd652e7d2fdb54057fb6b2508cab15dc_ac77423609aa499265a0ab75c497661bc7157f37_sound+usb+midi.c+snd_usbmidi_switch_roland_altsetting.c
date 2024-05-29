static void snd_usbmidi_switch_roland_altsetting(struct snd_usb_midi *umidi)
{
	struct usb_interface *intf;
	struct usb_host_interface *hostif;
	struct usb_interface_descriptor *intfd;

	intf = umidi->iface;
	if (!intf || intf->num_altsetting != 2)
		return;

	hostif = &intf->altsetting[1];
	intfd = get_iface_desc(hostif);
       /* If either or both of the endpoints support interrupt transfer,
        * then use the alternate setting
        */
	if (intfd->bNumEndpoints != 2 ||
	    !((get_endpoint(hostif, 0)->bmAttributes &
	       USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT ||
	      (get_endpoint(hostif, 1)->bmAttributes &
	       USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT))
		return;

	dev_dbg(&umidi->dev->dev, "switching to altsetting %d with int ep\n",
		    intfd->bAlternateSetting);
	usb_set_interface(umidi->dev, intfd->bInterfaceNumber,
			  intfd->bAlternateSetting);

	umidi->roland_load_ctl = snd_ctl_new1(&roland_load_ctl, umidi);
	if (snd_ctl_add(umidi->card, umidi->roland_load_ctl) < 0)
		umidi->roland_load_ctl = NULL;
}
