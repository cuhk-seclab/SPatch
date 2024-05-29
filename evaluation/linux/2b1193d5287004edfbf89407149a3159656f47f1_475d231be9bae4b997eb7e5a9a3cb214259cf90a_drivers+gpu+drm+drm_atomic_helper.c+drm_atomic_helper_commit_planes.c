void drm_atomic_helper_commit_planes(struct drm_device *dev,
				     struct drm_atomic_state *old_state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	struct drm_plane *plane;
	struct drm_plane_state *old_plane_state;
	int i;

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		funcs = crtc->helper_private;

		if (!funcs || !funcs->atomic_begin)
			continue;

		funcs->atomic_begin(crtc);
	}

	for_each_plane_in_state(old_state, plane, old_plane_state, i) {
		const struct drm_plane_helper_funcs *funcs;

		funcs = plane->helper_private;

		if (!funcs)
			continue;

		old_plane_state = old_state->plane_states[i];

		/*
		 * Special-case disabling the plane if drivers support it.
		 */
		if (drm_atomic_plane_disabling(plane, old_plane_state) &&
		    funcs->atomic_disable)
			funcs->atomic_disable(plane, old_plane_state);
		else if (plane->state->crtc ||
			 drm_atomic_plane_disabling(plane, old_plane_state))
			funcs->atomic_update(plane, old_plane_state);
	}

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		funcs = crtc->helper_private;

		if (!funcs || !funcs->atomic_flush)
			continue;

		funcs->atomic_flush(crtc);
	}
}
