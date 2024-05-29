static int rc_dev_uevent(struct device *device, struct kobj_uevent_env *env)
{
	struct rc_dev *dev = to_rc_dev(device);

	if (dev->rc_map.name)
		ADD_HOTPLUG_VAR("NAME=%s", dev->rc_map.name);
	if (dev->driver_name)
		ADD_HOTPLUG_VAR("DRV_NAME=%s", dev->driver_name);

	return 0;
}
