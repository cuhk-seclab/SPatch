int intel_pin_and_map_ringbuffer_obj(struct drm_device *dev,
				     struct intel_ringbuffer *ringbuf)
{
	struct drm_i915_private *dev_priv = to_i915(dev);
	struct drm_i915_gem_object *obj = ringbuf->obj;
	int ret;

	if (HAS_LLC(dev_priv) && !obj->stolen) {
		ret = i915_gem_obj_ggtt_pin(obj, PAGE_SIZE, 0);
		if (ret)
			return ret;

		ret = i915_gem_object_set_to_cpu_domain(obj, true);
		if (ret) {
			i915_gem_object_ggtt_unpin(obj);
			return ret;
		}

		ringbuf->virtual_start = vmap_obj(obj);
		if (ringbuf->virtual_start == NULL) {
			i915_gem_object_ggtt_unpin(obj);
			return -ENOMEM;
		}
	} else {
		ret = i915_gem_obj_ggtt_pin(obj, PAGE_SIZE, PIN_MAPPABLE);
		if (ret)
			return ret;

		ret = i915_gem_object_set_to_gtt_domain(obj, true);
		if (ret) {
			i915_gem_object_ggtt_unpin(obj);
			return ret;
		}

		ringbuf->virtual_start = ioremap_wc(dev_priv->gtt.mappable_base +
						    i915_gem_obj_ggtt_offset(obj), ringbuf->size);
		if (ringbuf->virtual_start == NULL) {
			i915_gem_object_ggtt_unpin(obj);
			return -EINVAL;
		}
	}

	return 0;
}
