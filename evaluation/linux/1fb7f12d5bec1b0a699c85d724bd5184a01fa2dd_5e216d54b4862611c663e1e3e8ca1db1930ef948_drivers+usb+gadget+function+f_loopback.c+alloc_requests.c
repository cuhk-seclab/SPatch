static int alloc_requests(struct usb_composite_dev *cdev,
			  struct f_loopback *loop)
{
	struct usb_request *in_req, *out_req;
	int i;
	int result = 0;

	/*
	 * allocate a bunch of read buffers and queue them all at once.
	 * we buffer at most 'qlen' transfers; We allocate buffers only
	 * for out transfer and reuse them in IN transfers to implement
	 * our loopback functionality
	 */
	for (i = 0; i < loop->qlen && result == 0; i++) {
		result = -ENOMEM;

		in_req = usb_ep_alloc_request(loop->in_ep, GFP_KERNEL);
		if (!in_req)
			goto fail;

		out_req = lb_alloc_ep_req(loop->out_ep, 0);
		if (!out_req)
			goto fail_in;

		in_req->complete = loopback_complete;
		out_req->complete = loopback_complete;

		in_req->buf = out_req->buf;
		/* length will be set in complete routine */
		in_req->context = out_req;
		out_req->context = in_req;

		result = usb_ep_queue(loop->out_ep, out_req, GFP_ATOMIC);
		if (result) {
			ERROR(cdev, "%s queue req --> %d\n",
					loop->out_ep->name, result);
			goto fail_out;
		}
	}

	return 0;

fail_out:
	free_ep_req(loop->out_ep, out_req);
fail_in:
	usb_ep_free_request(loop->in_ep, in_req);
fail:
	return result;
}
