int init_workarounds_ring(struct intel_engine_cs *ring)
{
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	WARN_ON(ring->id != RCS);

	dev_priv->workarounds.count = 0;

	if (IS_BROADWELL(dev))
		return bdw_init_workarounds(ring);

	if (IS_CHERRYVIEW(dev))
		return chv_init_workarounds(ring);

	if (IS_SKYLAKE(dev))
		return skl_init_workarounds(ring);

	if (IS_BROXTON(dev))
		return bxt_init_workarounds(ring);

	return 0;
}
