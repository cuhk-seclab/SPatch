static int check_ext_ctrls(struct v4l2_ext_controls *c, int allow_priv)
{
	__u32 i;

	/* zero the reserved fields */
	c->reserved[0] = c->reserved[1] = 0;
	for (i = 0; i < c->count; i++)
		c->controls[i].reserved2[0] = 0;

	/* V4L2_CID_PRIVATE_BASE cannot be used as control class
	   when using extended controls.
	   Only when passed in through VIDIOC_G_CTRL and VIDIOC_S_CTRL
	   is it allowed for backwards compatibility.
	 */
	if (!allow_priv && c->ctrl_class == V4L2_CID_PRIVATE_BASE)
		return 0;
	if (c->ctrl_class == 0)
		return 1;
	/* Check that all controls are from the same control class. */
	for (i = 0; i < c->count; i++) {
		if (V4L2_CTRL_ID2CLASS(c->controls[i].id) != c->ctrl_class) {
			c->error_idx = i;
			return 0;
		}
	}
	return 1;
}
