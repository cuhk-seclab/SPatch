static int get_fan_status(struct toshiba_acpi_dev *dev, u32 *status)
{
	u32 result = hci_read(dev, HCI_FAN, status);

	if (result == TOS_FAILURE)
		pr_err("ACPI call to get Fan status failed\n");
	else if (result == TOS_NOT_SUPPORTED)
		return -ENODEV;
	else if (result == TOS_SUCCESS)
		return 0;

	return -EIO;
}
