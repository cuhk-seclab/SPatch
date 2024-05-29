static int
afunc_set_alt(struct usb_function *fn, unsigned intf, unsigned alt)
{
	struct usb_composite_dev *cdev = fn->config->cdev;
	struct audio_dev *agdev = func_to_agdev(fn);
	struct snd_uac2_chip *uac2 = &agdev->uac2;
	struct usb_gadget *gadget = cdev->gadget;
	struct device *dev = &uac2->pdev.dev;
	struct usb_request *req;
	struct usb_ep *ep;
	struct uac2_rtd_params *prm;
	int req_len, i;

	/* No i/f has more than 2 alt settings */
	if (alt > 1) {
		dev_err(dev, "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (intf == agdev->ac_intf) {
		/* Control I/f has only 1 AltSetting - 0 */
		if (alt) {
			dev_err(dev, "%s:%d Error!\n", __func__, __LINE__);
			return -EINVAL;
		}
		return 0;
	}

	if (intf == agdev->as_out_intf) {
		ep = agdev->out_ep;
		prm = &uac2->c_prm;
		config_ep_by_speed(gadget, fn, ep);
		agdev->as_out_alt = alt;
		req_len = prm->max_psize;
	} else if (intf == agdev->as_in_intf) {
		struct f_uac2_opts *opts = agdev_to_uac2_opts(agdev);
		unsigned int factor, rate;
		struct usb_endpoint_descriptor *ep_desc;

		ep = agdev->in_ep;
		prm = &uac2->p_prm;
		config_ep_by_speed(gadget, fn, ep);
		agdev->as_in_alt = alt;

		/* pre-calculate the playback endpoint's interval */
		if (gadget->speed == USB_SPEED_FULL) {
			ep_desc = &fs_epin_desc;
			factor = 1000;
		} else {
			ep_desc = &hs_epin_desc;
			factor = 8000;
		}

		/* pre-compute some values for iso_complete() */
		uac2->p_framesize = opts->p_ssize *
				    num_channels(opts->p_chmask);
		rate = opts->p_srate * uac2->p_framesize;
		uac2->p_interval = factor / (1 << (ep_desc->bInterval - 1));
		uac2->p_pktsize = min_t(unsigned int, rate / uac2->p_interval,
					prm->max_psize);

		if (uac2->p_pktsize < prm->max_psize)
			uac2->p_pktsize_residue = rate % uac2->p_interval;
		else
			uac2->p_pktsize_residue = 0;

		req_len = uac2->p_pktsize;
		uac2->p_residue = 0;
	} else {
		dev_err(dev, "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (alt == 0) {
		free_ep(prm, ep);
		return 0;
	}

	prm->ep_enabled = true;
	usb_ep_enable(ep);

	for (i = 0; i < USB_XFERS; i++) {
		if (!prm->ureq[i].req) {
			req = usb_ep_alloc_request(ep, GFP_ATOMIC);
			if (req == NULL)
				return -ENOMEM;

			prm->ureq[i].req = req;
			prm->ureq[i].pp = prm;

			req->zero = 0;
			req->context = &prm->ureq[i];
			req->length = req_len;
			req->complete = agdev_iso_complete;
			req->buf = prm->rbuf + i * prm->max_psize;
		}

		if (usb_ep_queue(ep, prm->ureq[i].req, GFP_ATOMIC))
			dev_err(dev, "%s:%d Error!\n", __func__, __LINE__);
	}

	return 0;
}
