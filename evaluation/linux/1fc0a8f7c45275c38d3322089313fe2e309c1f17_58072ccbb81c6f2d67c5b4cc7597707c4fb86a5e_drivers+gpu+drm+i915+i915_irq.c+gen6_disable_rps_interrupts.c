void gen6_disable_rps_interrupts(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	spin_lock_irq(&dev_priv->irq_lock);
	dev_priv->rps.interrupts_enabled = false;
	spin_unlock_irq(&dev_priv->irq_lock);

	cancel_work_sync(&dev_priv->rps.work);

	spin_lock_irq(&dev_priv->irq_lock);

	I915_WRITE(GEN6_PMINTRMSK, gen6_sanitize_rps_pm_mask(dev_priv, ~0));

	__gen6_disable_pm_irq(dev_priv, dev_priv->pm_rps_events);
	I915_WRITE(gen6_pm_ier(dev_priv), I915_READ(gen6_pm_ier(dev_priv)) &
				~dev_priv->pm_rps_events);

	spin_unlock_irq(&dev_priv->irq_lock);

	synchronize_irq(dev->irq);

	spin_lock_irq(&dev_priv->irq_lock);

	I915_WRITE(gen6_pm_iir(dev_priv), dev_priv->pm_rps_events);
	I915_WRITE(gen6_pm_iir(dev_priv), dev_priv->pm_rps_events);

	dev_priv->rps.pm_iir = 0;

	spin_unlock_irq(&dev_priv->irq_lock);
}
