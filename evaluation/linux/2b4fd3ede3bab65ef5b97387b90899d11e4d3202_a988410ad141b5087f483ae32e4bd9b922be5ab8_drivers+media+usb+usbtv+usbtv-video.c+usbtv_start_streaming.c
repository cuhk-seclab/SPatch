static int usbtv_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct usbtv *usbtv = vb2_get_drv_priv(vq);

	if (usbtv->udev == NULL)
		return -ENODEV;

	usbtv->sequence = 0;
	return usbtv_start(usbtv);
}
