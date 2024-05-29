acpi_status
acpi_ev_update_gpe_enable_mask(struct acpi_gpe_event_info *gpe_event_info)
{
	struct acpi_gpe_register_info *gpe_register_info;
	u32 register_bit;

	ACPI_FUNCTION_TRACE(ev_update_gpe_enable_mask);

	gpe_register_info = gpe_event_info->register_info;
	if (!gpe_register_info) {
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	register_bit = acpi_hw_get_gpe_register_bit(gpe_event_info);

	/* Clear the run bit up front */

	ACPI_CLEAR_BIT(gpe_register_info->enable_for_run, register_bit);

	/* Set the mask bit only if there are references to this GPE */

	if (gpe_event_info->runtime_count) {
		ACPI_SET_BIT(gpe_register_info->enable_for_run,
			     (u8)register_bit);
	}
	gpe_register_info->enable_mask = gpe_register_info->enable_for_run;

	return_ACPI_STATUS(AE_OK);
}
