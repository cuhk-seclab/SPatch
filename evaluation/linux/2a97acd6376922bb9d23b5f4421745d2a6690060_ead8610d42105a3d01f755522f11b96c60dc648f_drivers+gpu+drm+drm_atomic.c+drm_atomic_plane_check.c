static int drm_atomic_plane_check(struct drm_plane *plane,
		struct drm_plane_state *state)
{
	unsigned int fb_width, fb_height;
	unsigned int i;

	/* either *both* CRTC and FB must be set, or neither */
	if (WARN_ON(state->crtc && !state->fb)) {
		DRM_DEBUG_ATOMIC("CRTC set but no FB\n");
		return -EINVAL;
	} else if (WARN_ON(state->fb && !state->crtc)) {
		DRM_DEBUG_ATOMIC("FB set but no CRTC\n");
		return -EINVAL;
	}

	/* if disabled, we don't care about the rest of the state: */
	if (!state->crtc)
		return 0;

	/* Check whether this plane is usable on this CRTC */
	if (!(plane->possible_crtcs & drm_crtc_mask(state->crtc))) {
		DRM_DEBUG_ATOMIC("Invalid crtc for plane\n");
		return -EINVAL;
	}

	/* Check whether this plane supports the fb pixel format. */
	for (i = 0; i < plane->format_count; i++)
	if (i == plane->format_count) {
		DRM_DEBUG_ATOMIC("Invalid pixel format %s\n",
				 drm_get_format_name(state->fb->pixel_format));
		return -EINVAL;
	}

	/* Give drivers some help against integer overflows */
	if (state->crtc_w > INT_MAX ||
	    state->crtc_x > INT_MAX - (int32_t) state->crtc_w ||
	    state->crtc_h > INT_MAX ||
	    state->crtc_y > INT_MAX - (int32_t) state->crtc_h) {
		DRM_DEBUG_ATOMIC("Invalid CRTC coordinates %ux%u+%d+%d\n",
				 state->crtc_w, state->crtc_h,
				 state->crtc_x, state->crtc_y);
		return -ERANGE;
	}

	fb_width = state->fb->width << 16;
	fb_height = state->fb->height << 16;

	/* Make sure source coordinates are inside the fb. */
	if (state->src_w > fb_width ||
	    state->src_x > fb_width - state->src_w ||
	    state->src_h > fb_height ||
	    state->src_y > fb_height - state->src_h) {
		DRM_DEBUG_ATOMIC("Invalid source coordinates "
				 "%u.%06ux%u.%06u+%u.%06u+%u.%06u\n",
				 state->src_w >> 16, ((state->src_w & 0xffff) * 15625) >> 10,
				 state->src_h >> 16, ((state->src_h & 0xffff) * 15625) >> 10,
				 state->src_x >> 16, ((state->src_x & 0xffff) * 15625) >> 10,
				 state->src_y >> 16, ((state->src_y & 0xffff) * 15625) >> 10);
		return -ENOSPC;
	}

	return 0;
}
