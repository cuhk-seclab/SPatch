void intel_set_memory_cxsr(struct drm_i915_private *dev_priv, bool enable)
{
	struct drm_device *dev = dev_priv->dev;
	u32 val;

	if (IS_VALLEYVIEW(dev)) {
		I915_WRITE(FW_BLC_SELF_VLV, enable ? FW_CSPWRDWNEN : 0);
		if (IS_CHERRYVIEW(dev))
			chv_set_memory_pm5(dev_priv, enable);
	} else if (IS_G4X(dev) || IS_CRESTLINE(dev)) {
		I915_WRITE(FW_BLC_SELF, enable ? FW_BLC_SELF_EN : 0);
	} else if (IS_PINEVIEW(dev)) {
		val = I915_READ(DSPFW3) & ~PINEVIEW_SELF_REFRESH_EN;
		val |= enable ? PINEVIEW_SELF_REFRESH_EN : 0;
		I915_WRITE(DSPFW3, val);
	} else if (IS_I945G(dev) || IS_I945GM(dev)) {
		val = enable ? _MASKED_BIT_ENABLE(FW_BLC_SELF_EN) :
			       _MASKED_BIT_DISABLE(FW_BLC_SELF_EN);
		I915_WRITE(FW_BLC_SELF, val);
	} else if (IS_I915GM(dev)) {
		val = enable ? _MASKED_BIT_ENABLE(INSTPM_SELF_EN) :
			       _MASKED_BIT_DISABLE(INSTPM_SELF_EN);
		I915_WRITE(INSTPM, val);
	} else {
		return;
	}

	DRM_DEBUG_KMS("memory self-refresh is %s\n",
		      enable ? "enabled" : "disabled");
}
