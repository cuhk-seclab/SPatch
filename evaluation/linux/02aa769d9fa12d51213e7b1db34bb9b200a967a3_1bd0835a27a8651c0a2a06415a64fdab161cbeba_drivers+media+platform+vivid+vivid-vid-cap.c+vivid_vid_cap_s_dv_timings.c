int vivid_vid_cap_s_dv_timings(struct file *file, void *_fh,
				    struct v4l2_dv_timings *timings)
{
	struct vivid_dev *dev = video_drvdata(file);

	if (!vivid_is_hdmi_cap(dev))
		return -ENODATA;
	if (!v4l2_find_dv_timings_cap(timings, &vivid_dv_timings_cap,
				0, NULL, NULL))
		return -EINVAL;
	if (v4l2_match_dv_timings(timings, &dev->dv_timings_cap, 0))
		return 0;
	if (vb2_is_busy(&dev->vb_vid_cap_q))
		return -EBUSY;
	dev->dv_timings_cap = *timings;
	vivid_update_format_cap(dev, false);
	return 0;
}
