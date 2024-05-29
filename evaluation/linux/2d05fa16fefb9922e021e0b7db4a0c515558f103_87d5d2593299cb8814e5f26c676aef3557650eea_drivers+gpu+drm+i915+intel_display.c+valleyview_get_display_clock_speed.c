static int valleyview_get_display_clock_speed(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 val;
	int divider;

	if (dev_priv->hpll_freq == 0)
		dev_priv->hpll_freq = valleyview_get_vco(dev_priv);

	mutex_lock(&dev_priv->sb_lock);
	val = vlv_cck_read(dev_priv, CCK_DISPLAY_CLOCK_CONTROL);
	mutex_unlock(&dev_priv->sb_lock);

	divider = val & CCK_FREQUENCY_VALUES;

	WARN((val & CCK_FREQUENCY_STATUS) !=
	     (divider << CCK_FREQUENCY_STATUS_SHIFT),
	     "cdclk change in progress\n");

	return DIV_ROUND_CLOSEST(dev_priv->hpll_freq << 1, divider + 1);
}
