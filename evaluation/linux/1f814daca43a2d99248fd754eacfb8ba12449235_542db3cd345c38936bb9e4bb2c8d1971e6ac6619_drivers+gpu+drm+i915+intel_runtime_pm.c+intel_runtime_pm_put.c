void intel_runtime_pm_put(struct drm_i915_private *dev_priv)
{
	struct drm_device *dev = dev_priv->dev;
	struct device *device = &dev->pdev->dev;

	assert_rpm_wakelock_held(dev_priv);
	atomic_dec(&dev_priv->pm.wakeref_count);

	pm_runtime_mark_last_busy(device);
	pm_runtime_put_autosuspend(device);
}
