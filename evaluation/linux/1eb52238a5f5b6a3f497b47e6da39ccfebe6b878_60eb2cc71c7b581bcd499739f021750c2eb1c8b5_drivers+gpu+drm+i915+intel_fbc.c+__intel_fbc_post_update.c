static void __intel_fbc_post_update(struct intel_crtc *crtc)
{
	struct drm_i915_private *dev_priv = crtc->base.dev->dev_private;
	struct intel_fbc *fbc = &dev_priv->fbc;
	struct intel_fbc_reg_params old_params;

	WARN_ON(!mutex_is_locked(&fbc->lock));

	if (!fbc->enabled || fbc->crtc != crtc)
		return;

	if (!intel_fbc_can_activate(crtc)) {
		WARN_ON(fbc->active);
		return;
	}

	old_params = fbc->params;
	intel_fbc_get_reg_params(crtc, &fbc->params);

	/* If the scanout has not changed, don't modify the FBC settings.
	 * Note that we make the fundamental assumption that the fb->obj
	 * cannot be unpinned (and have its GTT offset and fence revoked)
	 * without first being decoupled from the scanout and FBC disabled.
	 */
	if (fbc->active &&
	    intel_fbc_reg_params_equal(&old_params, &fbc->params))
		return;

	intel_fbc_deactivate(dev_priv);
	intel_fbc_schedule_activation(crtc);
	fbc->no_fbc_reason = "FBC enabled (active or scheduled)";
}
