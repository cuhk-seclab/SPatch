void radeon_driver_lastclose_kms(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;

	radeon_fbdev_restore_mode(rdev);
	vga_switcheroo_process_delayed_switch();
}
