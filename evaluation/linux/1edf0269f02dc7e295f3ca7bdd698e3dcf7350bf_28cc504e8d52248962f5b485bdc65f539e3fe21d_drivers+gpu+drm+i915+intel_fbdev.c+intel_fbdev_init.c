int intel_fbdev_init(struct drm_device *dev)
{
	struct intel_fbdev *ifbdev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	if (WARN_ON(INTEL_INFO(dev)->num_pipes == 0))
		return -ENODEV;

	ifbdev = kzalloc(sizeof(struct intel_fbdev), GFP_KERNEL);
	if (ifbdev == NULL)
		return -ENOMEM;

	drm_fb_helper_prepare(dev, &ifbdev->helper, &intel_fb_helper_funcs);

	if (!intel_fbdev_init_bios(dev, ifbdev))
		ifbdev->preferred_bpp = 32;

	ret = drm_fb_helper_init(dev, &ifbdev->helper,
				 INTEL_INFO(dev)->num_pipes, 4);
	if (ret) {
		kfree(ifbdev);
		return ret;
	}

	ifbdev->helper.atomic = true;

	dev_priv->fbdev = ifbdev;
	INIT_WORK(&dev_priv->fbdev_suspend_work, intel_fbdev_suspend_worker);

	drm_fb_helper_single_add_all_connectors(&ifbdev->helper);

	return 0;
}
