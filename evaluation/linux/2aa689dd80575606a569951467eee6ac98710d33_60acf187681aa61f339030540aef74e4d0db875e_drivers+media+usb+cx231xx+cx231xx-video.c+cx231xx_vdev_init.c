static struct video_device *cx231xx_vdev_init(struct cx231xx *dev,
		const struct video_device
		*template, const char *type_name)
{
	struct video_device *vfd;

	vfd = video_device_alloc();
	if (NULL == vfd)
		return NULL;

	*vfd = *template;
	vfd->v4l2_dev = &dev->v4l2_dev;
	vfd->release = video_device_release;
	vfd->lock = &dev->lock;

	snprintf(vfd->name, sizeof(vfd->name), "%s %s", dev->name, type_name);

	video_set_drvdata(vfd, dev);
	if (dev->tuner_type == TUNER_ABSENT) {
		v4l2_disable_ioctl(vfd, VIDIOC_G_FREQUENCY);
		v4l2_disable_ioctl(vfd, VIDIOC_S_FREQUENCY);
		v4l2_disable_ioctl(vfd, VIDIOC_G_TUNER);
		v4l2_disable_ioctl(vfd, VIDIOC_S_TUNER);
	}
	return vfd;
}
