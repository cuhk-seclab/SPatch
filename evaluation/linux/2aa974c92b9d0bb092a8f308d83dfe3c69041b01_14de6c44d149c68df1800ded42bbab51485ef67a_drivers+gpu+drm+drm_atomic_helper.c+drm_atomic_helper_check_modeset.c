int
drm_atomic_helper_check_modeset(struct drm_device *dev,
				struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *connector_state;
	int i, ret;

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		if (!drm_mode_equal(&crtc->state->mode, &crtc_state->mode)) {
			DRM_DEBUG_ATOMIC("[CRTC:%d:%s] mode changed\n",
					 crtc->base.id, crtc->name);
			crtc_state->mode_changed = true;
		}

		if (crtc->state->enable != crtc_state->enable) {
			DRM_DEBUG_ATOMIC("[CRTC:%d:%s] enable changed\n",
					 crtc->base.id, crtc->name);

			/*
			 * For clarity this assignment is done here, but
			 * enable == 0 is only true when there are no
			 * connectors and a NULL mode.
			 *
			 * The other way around is true as well. enable != 0
			 * iff connectors are attached and a mode is set.
			 */
			crtc_state->mode_changed = true;
			crtc_state->connectors_changed = true;
		}
	}

	for_each_connector_in_state(state, connector, connector_state, i) {
		/*
		 * This only sets crtc->mode_changed for routing changes,
		 * drivers must set crtc->mode_changed themselves when connector
		 * properties need to be updated.
		 */
		ret = update_connector_routing(state, i);
		if (ret)
			return ret;
	}

	/*
	 * After all the routing has been prepared we need to add in any
	 * connector which is itself unchanged, but who's crtc changes it's
	 * configuration. This must be done before calling mode_fixup in case a
	 * crtc only changed its mode but has the same set of connectors.
	 */
	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		bool has_connectors =
			!!crtc_state->connector_mask;

		/*
		 * We must set ->active_changed after walking connectors for
		 * otherwise an update that only changes active would result in
		 * a full modeset because update_connector_routing force that.
		 */
		if (crtc->state->active != crtc_state->active) {
			DRM_DEBUG_ATOMIC("[CRTC:%d:%s] active changed\n",
					 crtc->base.id, crtc->name);
			crtc_state->active_changed = true;
		}

		if (!drm_atomic_crtc_needs_modeset(crtc_state))
			continue;

		DRM_DEBUG_ATOMIC("[CRTC:%d:%s] needs all connectors, enable: %c, active: %c\n",
				 crtc->base.id, crtc->name,
				 crtc_state->enable ? 'y' : 'n',
				 crtc_state->active ? 'y' : 'n');

		ret = drm_atomic_add_affected_connectors(state, crtc);
		if (ret != 0)
			return ret;

		ret = drm_atomic_add_affected_planes(state, crtc);
		if (ret != 0)
			return ret;

		if (crtc_state->enable != has_connectors) {
			DRM_DEBUG_ATOMIC("[CRTC:%d:%s] enabled/connectors mismatch\n",
					 crtc->base.id, crtc->name);

			return -EINVAL;
		}
	}

	return mode_fixup(state);
}
