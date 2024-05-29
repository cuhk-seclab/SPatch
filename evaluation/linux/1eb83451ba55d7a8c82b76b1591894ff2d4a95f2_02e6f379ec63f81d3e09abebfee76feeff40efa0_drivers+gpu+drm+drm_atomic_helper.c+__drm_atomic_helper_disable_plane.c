int __drm_atomic_helper_disable_plane(struct drm_plane *plane,
		struct drm_plane_state *plane_state)
{
	int ret;

	ret = drm_atomic_set_crtc_for_plane(plane_state, NULL);
	if (ret != 0)
		return ret;

	drm_atomic_set_fb_for_plane(plane_state, NULL);
	plane_state->crtc_x = 0;
	plane_state->crtc_y = 0;
	plane_state->crtc_w = 0;
	plane_state->crtc_h = 0;
	plane_state->src_x = 0;
	plane_state->src_y = 0;
	plane_state->src_w = 0;
	plane_state->src_h = 0;

	return 0;
}
