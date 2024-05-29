static void trim_stale_devices(struct pci_dev *dev)
{
	struct acpi_device *adev = ACPI_COMPANION(&dev->dev);
	struct pci_bus *bus = dev->subordinate;
	bool alive = dev->ignore_hotplug;

	if (adev) {
		acpi_status status;
		unsigned long long sta;

		status = acpi_evaluate_integer(adev->handle, "_STA", NULL, &sta);
		alive = alive || (ACPI_SUCCESS(status) && device_status_valid(sta));
	}
	if (!alive)
		alive = pci_device_is_present(dev);

	if (!alive) {
		pci_stop_and_remove_bus_device(dev);
		if (adev)
			acpi_bus_trim(adev);
	} else if (bus) {
		struct pci_dev *child, *tmp;

		/* The device is a bridge. so check the bus below it. */
		pm_runtime_get_sync(&dev->dev);
		list_for_each_entry_safe_reverse(child, tmp, &bus->devices, bus_list)
			trim_stale_devices(child);

		pm_runtime_put(&dev->dev);
	}
}
