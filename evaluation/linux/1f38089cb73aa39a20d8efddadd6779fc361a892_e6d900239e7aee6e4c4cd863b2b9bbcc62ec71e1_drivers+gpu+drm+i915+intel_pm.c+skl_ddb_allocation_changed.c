static bool skl_ddb_allocation_changed(const struct skl_ddb_allocation *new_ddb,
				       const struct intel_crtc *intel_crtc)
{
	struct drm_device *dev = intel_crtc->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	const struct skl_ddb_allocation *cur_ddb = &dev_priv->wm.skl_hw.ddb;

	/*
	 * If ddb allocation of pipes changed, it may require recalculation of
	 * watermarks
	 */
	if (memcmp(new_ddb->pipe, cur_ddb->pipe, sizeof(new_ddb->pipe)))
		return true;

	return false;
}
