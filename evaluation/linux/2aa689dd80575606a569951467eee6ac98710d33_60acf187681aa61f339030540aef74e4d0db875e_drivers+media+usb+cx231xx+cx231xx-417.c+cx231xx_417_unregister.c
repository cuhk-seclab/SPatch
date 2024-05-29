void cx231xx_417_unregister(struct cx231xx *dev)
{
	dprintk(1, "%s()\n", __func__);
	dprintk(3, "%s()\n", __func__);

	if (video_is_registered(&dev->v4l_device)) {
		video_unregister_device(&dev->v4l_device);
		v4l2_ctrl_handler_free(&dev->mpeg_ctrl_handler.hdl);
	}
}
