static int
intel_modeset_pipe_config(struct drm_crtc *crtc,
			  struct drm_atomic_state *state,
			  struct intel_crtc_state *pipe_config)
{
	struct intel_encoder *encoder;
	struct drm_connector *connector;
	struct drm_connector_state *connector_state;
	int base_bpp, ret = -EINVAL;
	int i;
	bool retry = true;

	if (!check_encoder_cloning(state, to_intel_crtc(crtc))) {
		DRM_DEBUG_KMS("rejecting invalid cloning configuration\n");
		return -EINVAL;
	}

	if (!check_digital_port_conflicts(state)) {
		DRM_DEBUG_KMS("rejecting conflicting digital port configuration\n");
		return -EINVAL;
	}

	clear_intel_crtc_state(pipe_config);

	pipe_config->cpu_transcoder =
		(enum transcoder) to_intel_crtc(crtc)->pipe;

	/*
	 * Sanitize sync polarity flags based on requested ones. If neither
	 * positive or negative polarity is requested, treat this as meaning
	 * negative polarity.
	 */
	if (!(pipe_config->base.adjusted_mode.flags &
	      (DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NHSYNC)))
		pipe_config->base.adjusted_mode.flags |= DRM_MODE_FLAG_NHSYNC;

	if (!(pipe_config->base.adjusted_mode.flags &
	      (DRM_MODE_FLAG_PVSYNC | DRM_MODE_FLAG_NVSYNC)))
		pipe_config->base.adjusted_mode.flags |= DRM_MODE_FLAG_NVSYNC;

	/* Compute a starting value for pipe_config->pipe_bpp taking the source
	 * plane pixel format and any sink constraints into account. Returns the
	 * source plane bpp so that dithering can be selected on mismatches
	 * after encoders and crtc also have had their say. */
	base_bpp = compute_baseline_pipe_bpp(to_intel_crtc(crtc),
					     pipe_config);
	if (base_bpp < 0)
		goto fail;

	/*
	 * Determine the real pipe dimensions. Note that stereo modes can
	 * increase the actual pipe size due to the frame doubling and
	 * insertion of additional space for blanks between the frame. This
	 * is stored in the crtc timings. We use the requested mode to do this
	 * computation to clearly distinguish it from the adjusted mode, which
	 * can be changed by the connectors in the below retry loop.
	 */
	drm_crtc_get_hv_timing(&pipe_config->base.mode,
			       &pipe_config->pipe_src_w,
			       &pipe_config->pipe_src_h);

encoder_retry:
	/* Ensure the port clock defaults are reset when retrying. */
	pipe_config->port_clock = 0;
	pipe_config->pixel_multiplier = 1;

	/* Fill in default crtc timings, allow encoders to overwrite them. */
	drm_mode_set_crtcinfo(&pipe_config->base.adjusted_mode,
			      CRTC_STEREO_DOUBLE);

	/* Pass our mode to the connectors and the CRTC to give them a chance to
	 * adjust it according to limitations or connector properties, and also
	 * a chance to reject the mode entirely.
	 */
	for_each_connector_in_state(state, connector, connector_state, i) {
		if (connector_state->crtc != crtc)
			continue;

		encoder = to_intel_encoder(connector_state->best_encoder);

		if (!(encoder->compute_config(encoder, pipe_config))) {
			DRM_DEBUG_KMS("Encoder config failure\n");
			goto fail;
		}
	}

	/* Set default port clock if not overwritten by the encoder. Needs to be
	 * done afterwards in case the encoder adjusts the mode. */
	if (!pipe_config->port_clock)
		pipe_config->port_clock = pipe_config->base.adjusted_mode.crtc_clock
			* pipe_config->pixel_multiplier;

	ret = intel_crtc_compute_config(to_intel_crtc(crtc), pipe_config);
	if (ret < 0) {
		DRM_DEBUG_KMS("CRTC fixup failed\n");
		goto fail;
	}

	if (ret == RETRY) {
		if (WARN(!retry, "loop in pipe configuration computation\n")) {
			ret = -EINVAL;
			goto fail;
		}

		DRM_DEBUG_KMS("CRTC bw constrained, retrying\n");
		retry = false;
		goto encoder_retry;
	}

	pipe_config->dither = pipe_config->pipe_bpp != base_bpp;
	DRM_DEBUG_KMS("plane bpp: %i, pipe bpp: %i, dithering: %i\n",
		      base_bpp, pipe_config->pipe_bpp, pipe_config->dither);

	return 0;
fail:
	return ret;
}
