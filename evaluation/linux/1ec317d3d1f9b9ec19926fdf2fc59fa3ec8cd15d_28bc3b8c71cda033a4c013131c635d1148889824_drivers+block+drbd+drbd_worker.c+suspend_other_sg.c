void suspend_other_sg(struct drbd_device *device)
{
	lock_all_resources();
	drbd_pause_after(device);
	unlock_all_resources();
}
