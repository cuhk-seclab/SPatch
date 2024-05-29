acpi_status
acpi_ds_initialize_objects(u32 table_index,
			   struct acpi_namespace_node * start_node)
{
	acpi_status status;
	struct acpi_init_walk_info info;
	struct acpi_table_header *table;
	acpi_owner_id owner_id;

	ACPI_FUNCTION_TRACE(ds_initialize_objects);

	status = acpi_tb_get_owner_id(table_index, &owner_id);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_DISPATCH,
			  "**** Starting initialization of namespace objects ****\n"));

	/* Set all init info to zero */

	memset(&info, 0, sizeof(struct acpi_init_walk_info));

	info.owner_id = owner_id;
	info.table_index = table_index;

	/* Walk entire namespace from the supplied root */

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * We don't use acpi_walk_namespace since we do not want to acquire
	 * the namespace reader lock.
	 */
	status =
	    acpi_ns_walk_namespace(ACPI_TYPE_ANY, start_node, ACPI_UINT32_MAX,
				   ACPI_NS_WALK_UNLOCK, acpi_ds_init_one_object,
				   NULL, &info, NULL);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "During WalkNamespace"));
	}
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);

	status = acpi_get_table_by_index(table_index, &table);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* DSDT is always the first AML table */

	if (ACPI_COMPARE_NAME(table->signature, ACPI_SIG_DSDT)) {
		ACPI_DEBUG_PRINT_RAW((ACPI_DB_INIT,
				      "\nInitializing Namespace objects:\n"));
	}

	/* Summary of objects initialized */

	ACPI_DEBUG_PRINT_RAW((ACPI_DB_INIT,
			      "Table [%4.4s] (id %4.4X) - %4u Objects with %3u Devices, "
			      "%3u Regions, %3u Methods (%u/%u/%u Serial/Non/Cvt)\n",
			      table->signature, owner_id, info.object_count,
			      info.device_count, info.op_region_count,
			      info.method_count, info.serial_method_count,
			      info.non_serial_method_count,
			      info.serialized_method_count));

	ACPI_DEBUG_PRINT((ACPI_DB_DISPATCH, "%u Methods, %u Regions\n",
			  info.method_count, info.op_region_count));

	return_ACPI_STATUS(AE_OK);
}
