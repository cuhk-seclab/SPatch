static int intel_atomic_check(struct drm_device *dev,
			      struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	int ret, i;
	bool any_ms = false;

	ret = drm_atomic_helper_check_modeset(dev, state);
	if (ret)
		return ret;

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		struct intel_crtc_state *pipe_config =
			to_intel_crtc_state(crtc_state);

		/* Catch I915_MODE_FLAG_INHERITED */
		if (crtc_state->mode.private_flags != crtc->state->mode.private_flags)
			crtc_state->mode_changed = true;

		if (!crtc_state->enable) {
			if (needs_modeset(crtc_state))
				any_ms = true;
			continue;
		}

		if (!needs_modeset(crtc_state))
			continue;

		/* FIXME: For only active_changed we shouldn't need to do any
		 * state recomputation at all. */

		ret = drm_atomic_add_affected_connectors(state, crtc);
		if (ret)
			return ret;

		ret = intel_modeset_pipe_config(crtc, pipe_config);
		if (ret)
			return ret;

		if (i915.fastboot &&
		    intel_pipe_config_compare(state->dev,
					to_intel_crtc_state(crtc->state),
					pipe_config, true)) {
			crtc_state->mode_changed = false;
		}

		if (needs_modeset(crtc_state)) {
			any_ms = true;

			ret = drm_atomic_add_affected_planes(state, crtc);
			if (ret)
				return ret;
		}

		intel_dump_pipe_config(to_intel_crtc(crtc), pipe_config,
				       needs_modeset(crtc_state) ?
				       "[modeset]" : "[fastset]");
	}

	if (any_ms) {
		ret = intel_modeset_checks(state);

		if (ret)
			return ret;
	} else
		to_intel_atomic_state(state)->cdclk =
			to_i915(state->dev)->cdclk_freq;

	return drm_atomic_helper_check_planes(state->dev, state);
}
