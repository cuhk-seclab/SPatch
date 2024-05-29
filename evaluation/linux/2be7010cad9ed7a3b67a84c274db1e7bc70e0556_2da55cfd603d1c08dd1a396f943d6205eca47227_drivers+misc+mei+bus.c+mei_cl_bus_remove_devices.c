void mei_cl_bus_remove_devices(struct mei_device *bus)
{
	struct mei_cl_device *cldev, *next;

	mutex_lock(&bus->cl_bus_lock);
	list_for_each_entry_safe(cldev, next, &bus->device_list, bus_list)
		mei_cl_bus_remove_device(cldev);
	mutex_unlock(&bus->cl_bus_lock);
}
