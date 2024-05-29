int vivid_vid_out_s_dv_timings(struct file *file, void *_fh,
				    struct v4l2_dv_timings *timings)
{
	struct vivid_dev *dev = video_drvdata(file);

	if (!vivid_is_hdmi_out(dev))
		return -ENODATA;
	if (!v4l2_find_dv_timings_cap(timings, &vivid_dv_timings_cap,
				0, NULL, NULL))
		return -EINVAL;
	if (v4l2_match_dv_timings(timings, &dev->dv_timings_out, 0))
		return 0;
	if (vb2_is_busy(&dev->vb_vid_out_q))
		return -EBUSY;
	dev->dv_timings_out = *timings;
	vivid_update_format_out(dev);
	return 0;
}
