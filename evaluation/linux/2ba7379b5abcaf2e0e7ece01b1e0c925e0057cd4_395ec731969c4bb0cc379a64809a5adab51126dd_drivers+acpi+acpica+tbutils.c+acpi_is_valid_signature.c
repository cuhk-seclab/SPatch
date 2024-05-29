u8 acpi_is_valid_signature(char *signature)
{
	u32 i;

	/* Validate each character in the signature */

	for (i = 0; i < ACPI_NAME_SIZE; i++) {
		if (!acpi_ut_valid_acpi_char(signature[i], i)) {
			return (FALSE);
		}
	}

	return (TRUE);
}
