static ssize_t
acpi_device_modalias_show(struct device *dev, struct device_attribute *attr, char *buf) {
	return __acpi_device_modalias(to_acpi_device(dev), buf, 1024);
}
