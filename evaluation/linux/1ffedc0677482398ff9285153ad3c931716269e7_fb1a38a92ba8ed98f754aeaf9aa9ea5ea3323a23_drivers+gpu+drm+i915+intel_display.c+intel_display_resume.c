void intel_display_resume(struct drm_device *dev)
{
	struct drm_atomic_state *state = drm_atomic_state_alloc(dev);
	struct intel_connector *conn;
	struct intel_plane *plane;
	struct drm_crtc *crtc;
	int ret;

	if (!state)
		return;

	state->acquire_ctx = dev->mode_config.acquire_ctx;

	for_each_crtc(dev, crtc) {
		struct drm_crtc_state *crtc_state =
			drm_atomic_get_crtc_state(state, crtc);

		ret = PTR_ERR_OR_ZERO(crtc_state);
		if (ret)
			goto err;

		/* force a restore */
		crtc_state->mode_changed = true;
	}

	for_each_intel_plane(dev, plane) {
		ret = PTR_ERR_OR_ZERO(drm_atomic_get_plane_state(state, &plane->base));
		if (ret)
			goto err;
	}

	for_each_intel_connector(dev, conn) {
		ret = PTR_ERR_OR_ZERO(drm_atomic_get_connector_state(state, &conn->base));
		if (ret)
			goto err;
	}

	intel_modeset_setup_hw_state(dev);

	i915_redisable_vga(dev);
	ret = drm_atomic_commit(state);
	if (!ret)
		return;

err:
	DRM_ERROR("Restoring old state failed with %i\n", ret);
	drm_atomic_state_free(state);
}
