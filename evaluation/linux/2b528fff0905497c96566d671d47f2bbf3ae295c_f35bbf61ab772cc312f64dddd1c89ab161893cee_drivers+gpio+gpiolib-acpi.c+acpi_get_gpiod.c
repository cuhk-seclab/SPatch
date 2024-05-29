static struct gpio_desc *acpi_get_gpiod(char *path, int pin)
{
	struct gpio_chip *chip;
	acpi_handle handle;
	acpi_status status;
	int offset;

	status = acpi_get_handle(NULL, path, &handle);
	if (ACPI_FAILURE(status))
		return ERR_PTR(-ENODEV);

	chip = gpiochip_find(handle, acpi_gpiochip_find);
	if (!chip)
		return ERR_PTR(-EPROBE_DEFER);

	offset = acpi_gpiochip_pin_to_gpio_offset(chip, pin);
	if (offset < 0)
		return ERR_PTR(offset);

	return gpiochip_get_desc(chip, offset);
}
