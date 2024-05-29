int
intel_pin_and_fence_fb_obj(struct drm_plane *plane,
			   struct drm_framebuffer *fb,
			   const struct drm_plane_state *plane_state,
			   struct intel_engine_cs *pipelined,
			   struct drm_i915_gem_request **pipelined_request)
{
	struct drm_device *dev = fb->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj = intel_fb_obj(fb);
	struct i915_ggtt_view view;
	u32 alignment;
	int ret;

	WARN_ON(!mutex_is_locked(&dev->struct_mutex));

	switch (fb->modifier[0]) {
	case DRM_FORMAT_MOD_NONE:
		alignment = intel_linear_alignment(dev_priv);
		break;
	case I915_FORMAT_MOD_X_TILED:
		if (INTEL_INFO(dev)->gen >= 9)
			alignment = 256 * 1024;
		else {
			/* pin() will align the object as required by fence */
			alignment = 0;
		}
		break;
	case I915_FORMAT_MOD_Y_TILED:
	case I915_FORMAT_MOD_Yf_TILED:
		if (WARN_ONCE(INTEL_INFO(dev)->gen < 9,
			  "Y tiling bo slipped through, driver bug!\n"))
			return -EINVAL;
		alignment = 1 * 1024 * 1024;
		break;
	default:
		MISSING_CASE(fb->modifier[0]);
		return -EINVAL;
	}

	ret = intel_fill_fb_ggtt_view(&view, fb, plane_state);
	if (ret)
		return ret;

	/* Note that the w/a also requires 64 PTE of padding following the
	 * bo. We currently fill all unused PTE with the shadow page and so
	 * we should always have valid PTE following the scanout preventing
	 * the VT-d warning.
	 */
	if (need_vtd_wa(dev) && alignment < 256 * 1024)
		alignment = 256 * 1024;

	/*
	 * Global gtt pte registers are special registers which actually forward
	 * writes to a chunk of system memory. Which means that there is no risk
	 * that the register values disappear as soon as we call
	 * intel_runtime_pm_put(), so it is correct to wrap only the
	 * pin/unpin/fence and not more.
	 */
	intel_runtime_pm_get(dev_priv);

	ret = i915_gem_object_pin_to_display_plane(obj, alignment, pipelined,
						   pipelined_request, &view);
	if (ret)
		goto err_pm;

	/* Install a fence for tiled scan-out. Pre-i965 always needs a
	 * fence, whereas 965+ only requires a fence if using
	 * framebuffer compression.  For simplicity, we always install
	 * a fence as the cost is not that onerous.
	 */
	ret = i915_gem_object_get_fence(obj);
	if (ret == -EDEADLK) {
		/*
		 * -EDEADLK means there are no free fences
		 * no pending flips.
		 *
		 * This is propagated to atomic, but it uses
		 * -EDEADLK to force a locking recovery, so
		 * change the returned error to -EBUSY.
		 */
		ret = -EBUSY;
		goto err_unpin;
	} else if (ret)
		goto err_unpin;

	i915_gem_object_pin_fence(obj);

	intel_runtime_pm_put(dev_priv);
	return 0;

err_unpin:
	i915_gem_object_unpin_from_display_plane(obj, &view);
err_pm:
	intel_runtime_pm_put(dev_priv);
	return ret;
}
