static const char *get_pin_flag(struct drm_i915_gem_object *obj)
{
	if (obj->pin_display)
		return "p";
	else
		return " ";
}
