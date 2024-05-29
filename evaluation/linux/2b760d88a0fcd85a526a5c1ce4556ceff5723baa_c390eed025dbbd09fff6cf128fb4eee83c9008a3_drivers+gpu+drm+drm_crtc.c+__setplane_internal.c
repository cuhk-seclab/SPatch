static int __setplane_internal(struct drm_plane *plane,
			       struct drm_crtc *crtc,
			       struct drm_framebuffer *fb,
			       int32_t crtc_x, int32_t crtc_y,
			       uint32_t crtc_w, uint32_t crtc_h,
			       /* src_{x,y,w,h} values are 16.16 fixed point */
			       uint32_t src_x, uint32_t src_y,
			       uint32_t src_w, uint32_t src_h)
{
	int ret = 0;
	unsigned int fb_width, fb_height;

	/* No fb means shut it down */
	if (!fb) {
		plane->old_fb = plane->fb;
		ret = plane->funcs->disable_plane(plane);
		if (!ret) {
			plane->crtc = NULL;
			plane->fb = NULL;
		} else {
			plane->old_fb = NULL;
		}
		goto out;
	}

	/* Check whether this plane is usable on this CRTC */
	if (!(plane->possible_crtcs & drm_crtc_mask(crtc))) {
		DRM_DEBUG_KMS("Invalid crtc for plane\n");
		ret = -EINVAL;
		goto out;
	}

	/* Check whether this plane supports the fb pixel format. */
	ret = drm_plane_check_pixel_format(plane, fb->pixel_format);
	if (ret) {
		DRM_DEBUG_KMS("Invalid pixel format %s\n",
			      drm_get_format_name(fb->pixel_format));
		goto out;
	}

	/* Give drivers some help against integer overflows */
	if (crtc_w > INT_MAX ||
	    crtc_x > INT_MAX - (int32_t) crtc_w ||
	    crtc_h > INT_MAX ||
	    crtc_y > INT_MAX - (int32_t) crtc_h) {
		DRM_DEBUG_KMS("Invalid CRTC coordinates %ux%u+%d+%d\n",
			      crtc_w, crtc_h, crtc_x, crtc_y);
		ret = -ERANGE;
		goto out;
	}


	fb_width = fb->width << 16;
	fb_height = fb->height << 16;

	/* Make sure source coordinates are inside the fb. */
	if (src_w > fb_width ||
	    src_x > fb_width - src_w ||
	    src_h > fb_height ||
	    src_y > fb_height - src_h) {
		DRM_DEBUG_KMS("Invalid source coordinates "
			      "%u.%06ux%u.%06u+%u.%06u+%u.%06u\n",
			      src_w >> 16, ((src_w & 0xffff) * 15625) >> 10,
			      src_h >> 16, ((src_h & 0xffff) * 15625) >> 10,
			      src_x >> 16, ((src_x & 0xffff) * 15625) >> 10,
			      src_y >> 16, ((src_y & 0xffff) * 15625) >> 10);
		ret = -ENOSPC;
		goto out;
	}

	plane->old_fb = plane->fb;
	ret = plane->funcs->update_plane(plane, crtc, fb,
					 crtc_x, crtc_y, crtc_w, crtc_h,
					 src_x, src_y, src_w, src_h);
	if (!ret) {
		plane->crtc = crtc;
		plane->fb = fb;
		fb = NULL;
	} else {
		plane->old_fb = NULL;
	}

out:
	if (fb)
		drm_framebuffer_unreference(fb);
	if (plane->old_fb)
		drm_framebuffer_unreference(plane->old_fb);
	plane->old_fb = NULL;

	return ret;
}
