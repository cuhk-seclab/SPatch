static int
intel_modeset_stage_output_state(struct drm_device *dev,
				 struct drm_mode_set *set,
				 struct drm_atomic_state *state)
{
	struct intel_connector *connector;
	struct drm_connector *drm_connector;
	struct drm_connector_state *connector_state;
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	int i, ret;

	/* The upper layers ensure that we either disable a crtc or have a list
	 * of connectors. For paranoia, double-check this. */
	WARN_ON(!set->fb && (set->num_connectors != 0));
	WARN_ON(set->fb && (set->num_connectors == 0));

	for_each_intel_connector(dev, connector) {
		bool in_mode_set = intel_connector_in_mode_set(connector, set);

		if (!in_mode_set && connector->base.state->crtc != set->crtc)
			continue;

		connector_state =
			drm_atomic_get_connector_state(state, &connector->base);
		if (IS_ERR(connector_state))
			return PTR_ERR(connector_state);

		if (in_mode_set) {
			int pipe = to_intel_crtc(set->crtc)->pipe;
			connector_state->best_encoder =
				&intel_find_encoder(connector, pipe)->base;
		}

		if (connector->base.state->crtc != set->crtc)
			continue;

		/* If we disable the crtc, disable all its connectors. Also, if
		 * the connector is on the changing crtc but not on the new
		 * connector list, disable it. */
		if (!set->fb || !in_mode_set) {
			connector_state->best_encoder = NULL;

			DRM_DEBUG_KMS("[CONNECTOR:%d:%s] to [NOCRTC]\n",
				connector->base.base.id,
				connector->base.name);
		}
	}
	/* connector->new_encoder is now updated for all connectors. */

	for_each_connector_in_state(state, drm_connector, connector_state, i) {
		connector = to_intel_connector(drm_connector);

		if (!connector_state->best_encoder) {
			ret = drm_atomic_set_crtc_for_connector(connector_state,
								NULL);
			if (ret)
				return ret;

			continue;
		}

		if (intel_connector_in_mode_set(connector, set)) {
			struct drm_crtc *crtc = connector->base.state->crtc;

			/* If this connector was in a previous crtc, add it
			 * to the state. We might need to disable it. */
			if (crtc) {
				crtc_state =
					drm_atomic_get_crtc_state(state, crtc);
				if (IS_ERR(crtc_state))
					return PTR_ERR(crtc_state);
			}

			ret = drm_atomic_set_crtc_for_connector(connector_state,
								set->crtc);
			if (ret)
				return ret;
		}

		/* Make sure the new CRTC will work with the encoder */
		if (!drm_encoder_crtc_ok(connector_state->best_encoder,
					 connector_state->crtc)) {
			return -EINVAL;
		}

		DRM_DEBUG_KMS("[CONNECTOR:%d:%s] to [CRTC:%d]\n",
			connector->base.base.id,
			connector->base.name,
			connector_state->crtc->base.id);

		if (connector_state->best_encoder != &connector->encoder->base)
			connector->encoder =
				to_intel_encoder(connector_state->best_encoder);
	}

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		bool has_connectors;

		ret = drm_atomic_add_affected_connectors(state, crtc);
		if (ret)
			return ret;

		has_connectors = !!drm_atomic_connectors_for_crtc(state, crtc);
		if (has_connectors != crtc_state->enable)
			crtc_state->enable =
			crtc_state->active = has_connectors;
	}

	ret = intel_modeset_setup_plane_state(state, set->crtc, set->mode,
					      set->fb, set->x, set->y);
	if (ret)
		return ret;

	crtc_state = drm_atomic_get_crtc_state(state, set->crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	if (set->mode)
		drm_mode_copy(&crtc_state->mode, set->mode);

	if (set->num_connectors)
		crtc_state->active = true;

	return 0;
}
