int drm_mode_page_flip_ioctl(struct drm_device *dev,
			     void *data, struct drm_file *file_priv)
{
	struct drm_mode_crtc_page_flip *page_flip = data;
	struct drm_crtc *crtc;
	struct drm_framebuffer *fb = NULL;
	struct drm_pending_vblank_event *e = NULL;
	unsigned long flags;
	int ret = -EINVAL;

	if (page_flip->flags & ~DRM_MODE_PAGE_FLIP_FLAGS ||
	    page_flip->reserved != 0)
		return -EINVAL;

	if ((page_flip->flags & DRM_MODE_PAGE_FLIP_ASYNC) && !dev->mode_config.async_page_flip)
		return -EINVAL;

	crtc = drm_crtc_find(dev, page_flip->crtc_id);
	if (!crtc)
		return -ENOENT;

	drm_modeset_lock_crtc(crtc, crtc->primary);
	if (crtc->primary->fb == NULL) {
		/* The framebuffer is currently unbound, presumably
		 * due to a hotplug event, that userspace has not
		 * yet discovered.
		 */
		ret = -EBUSY;
		goto out;
	}

	if (crtc->funcs->page_flip == NULL)
		goto out;

	fb = drm_framebuffer_lookup(dev, page_flip->fb_id);
	if (!fb) {
		ret = -ENOENT;
		goto out;
	}

	ret = drm_crtc_check_viewport(crtc, crtc->x, crtc->y, &crtc->mode, fb);
	if (ret)
		goto out;

	if (crtc->primary->fb->pixel_format != fb->pixel_format) {
		DRM_DEBUG_KMS("Page flip is not allowed to change frame buffer format.\n");
		ret = -EINVAL;
		goto out;
	}

	if (page_flip->flags & DRM_MODE_PAGE_FLIP_EVENT) {
		ret = -ENOMEM;
		spin_lock_irqsave(&dev->event_lock, flags);
		if (file_priv->event_space < sizeof(e->event)) {
			spin_unlock_irqrestore(&dev->event_lock, flags);
			goto out;
		}
		file_priv->event_space -= sizeof(e->event);
		spin_unlock_irqrestore(&dev->event_lock, flags);

		e = kzalloc(sizeof(*e), GFP_KERNEL);
		if (e == NULL) {
			spin_lock_irqsave(&dev->event_lock, flags);
			file_priv->event_space += sizeof(e->event);
			spin_unlock_irqrestore(&dev->event_lock, flags);
			goto out;
		}

		e->event.base.type = DRM_EVENT_FLIP_COMPLETE;
		e->event.base.length = sizeof(e->event);
		e->event.user_data = page_flip->user_data;
		e->base.event = &e->event.base;
		e->base.file_priv = file_priv;
		e->base.destroy =
			(void (*) (struct drm_pending_event *)) kfree;
	}

	crtc->primary->old_fb = crtc->primary->fb;
	ret = crtc->funcs->page_flip(crtc, fb, e, page_flip->flags);
	if (ret) {
		if (page_flip->flags & DRM_MODE_PAGE_FLIP_EVENT) {
			spin_lock_irqsave(&dev->event_lock, flags);
			file_priv->event_space += sizeof(e->event);
			spin_unlock_irqrestore(&dev->event_lock, flags);
			kfree(e);
		}
		/* Keep the old fb, don't unref it. */
		crtc->primary->old_fb = NULL;
	} else {
		crtc->primary->fb = fb;
		/* Unref only the old framebuffer. */
		fb = NULL;
	}

out:
	if (fb)
		drm_framebuffer_unreference(fb);
	if (crtc->primary->old_fb)
		drm_framebuffer_unreference(crtc->primary->old_fb);
	crtc->primary->old_fb = NULL;
	drm_modeset_unlock_crtc(crtc);

	return ret;
}
