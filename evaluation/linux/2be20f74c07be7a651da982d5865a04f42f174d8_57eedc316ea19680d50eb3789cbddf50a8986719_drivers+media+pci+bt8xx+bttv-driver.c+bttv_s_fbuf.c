static int bttv_s_fbuf(struct file *file, void *f,
				const struct v4l2_framebuffer *fb)
{
	struct bttv_fh *fh = f;
	struct bttv *btv = fh->btv;
	const struct bttv_format *fmt;
	int retval;

	if (!capable(CAP_SYS_ADMIN) &&
		!capable(CAP_SYS_RAWIO))
		return -EPERM;

	/* check args */
	fmt = format_by_fourcc(fb->fmt.pixelformat);
	if (NULL == fmt)
		return -EINVAL;
	if (0 == (fmt->flags & FORMAT_FLAGS_PACKED))
		return -EINVAL;

	retval = -EINVAL;
	if (fb->flags & V4L2_FBUF_FLAG_OVERLAY) {
		__s32 width = fb->fmt.width;
		__s32 height = fb->fmt.height;

		retval = limit_scaled_size_lock(fh, &width, &height,
					   V4L2_FIELD_INTERLACED,
					   /* width_mask */ ~3,
					   /* width_bias */ 2,
					   /* adjust_size */ 0,
					   /* adjust_crop */ 0);
		if (0 != retval)
			return retval;
	}

	/* ok, accept it */
	btv->fbuf.base       = fb->base;
	btv->fbuf.fmt.width  = fb->fmt.width;
	btv->fbuf.fmt.height = fb->fmt.height;
	if (0 != fb->fmt.bytesperline)
		btv->fbuf.fmt.bytesperline = fb->fmt.bytesperline;
	else
		btv->fbuf.fmt.bytesperline = btv->fbuf.fmt.width*fmt->depth/8;

	retval = 0;
	fh->ovfmt = fmt;
	btv->init.ovfmt = fmt;
	if (fb->flags & V4L2_FBUF_FLAG_OVERLAY) {
		fh->ov.w.left   = 0;
		fh->ov.w.top    = 0;
		fh->ov.w.width  = fb->fmt.width;
		fh->ov.w.height = fb->fmt.height;
		btv->init.ov.w.width  = fb->fmt.width;
		btv->init.ov.w.height = fb->fmt.height;

		kfree(fh->ov.clips);
		fh->ov.clips = NULL;
		fh->ov.nclips = 0;

		if (check_btres(fh, RESOURCE_OVERLAY)) {
			struct bttv_buffer *new;

			new = videobuf_sg_alloc(sizeof(*new));
			new->crop = btv->crop[!!fh->do_crop].rect;
			bttv_overlay_risc(btv, &fh->ov, fh->ovfmt, new);
			retval = bttv_switch_overlay(btv, fh, new);
		}
	}
	return retval;
}
