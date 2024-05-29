static acpi_status acpi_tb_load_namespace(void)
{
	acpi_status status;
	u32 i;
	struct acpi_table_header *new_dsdt;
	u32 tables_loaded = 0;
	u32 tables_failed = 0;

	ACPI_FUNCTION_TRACE(tb_load_namespace);

	(void)acpi_ut_acquire_mutex(ACPI_MTX_TABLES);

	/*
	 * Load the namespace. The DSDT is required, but any SSDT and
	 * PSDT tables are optional. Verify the DSDT.
	 */
	if (!acpi_gbl_root_table_list.current_table_count ||
	    !ACPI_COMPARE_NAME(&
			       (acpi_gbl_root_table_list.
				tables[ACPI_TABLE_INDEX_DSDT].signature),
			       ACPI_SIG_DSDT)
	    ||
	    ACPI_FAILURE(acpi_tb_validate_table
			 (&acpi_gbl_root_table_list.
			  tables[ACPI_TABLE_INDEX_DSDT]))) {
		status = AE_NO_ACPI_TABLES;
		goto unlock_and_exit;
	}

	/*
	 * Save the DSDT pointer for simple access. This is the mapped memory
	 * address. We must take care here because the address of the .Tables
	 * array can change dynamically as tables are loaded at run-time. Note:
	 * .Pointer field is not validated until after call to acpi_tb_validate_table.
	 */
	acpi_gbl_DSDT =
	    acpi_gbl_root_table_list.tables[ACPI_TABLE_INDEX_DSDT].pointer;

	/*
	 * Optionally copy the entire DSDT to local memory (instead of simply
	 * mapping it.) There are some BIOSs that corrupt or replace the original
	 * DSDT, creating the need for this option. Default is FALSE, do not copy
	 * the DSDT.
	 */
	if (acpi_gbl_copy_dsdt_locally) {
		new_dsdt = acpi_tb_copy_dsdt(ACPI_TABLE_INDEX_DSDT);
		if (new_dsdt) {
			acpi_gbl_DSDT = new_dsdt;
		}
	}

	/*
	 * Save the original DSDT header for detection of table corruption
	 * and/or replacement of the DSDT from outside the OS.
	 */
	memcpy(&acpi_gbl_original_dsdt_header, acpi_gbl_DSDT,
	       sizeof(struct acpi_table_header));

	(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);

	/* Load and parse tables */

	status = acpi_ns_load_table(ACPI_TABLE_INDEX_DSDT, acpi_gbl_root_node);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "[DSDT] table load failed"));
		tables_failed++;
	} else {
		tables_loaded++;
	}

	/* Load any SSDT or PSDT tables. Note: Loop leaves tables locked */

	(void)acpi_ut_acquire_mutex(ACPI_MTX_TABLES);
	for (i = 0; i < acpi_gbl_root_table_list.current_table_count; ++i) {
		if (!acpi_gbl_root_table_list.tables[i].address ||
		    (!ACPI_COMPARE_NAME
		     (&(acpi_gbl_root_table_list.tables[i].signature),
		      ACPI_SIG_SSDT)
		     &&
		     !ACPI_COMPARE_NAME(&
					(acpi_gbl_root_table_list.tables[i].
					 signature), ACPI_SIG_PSDT)
		     &&
		     !ACPI_COMPARE_NAME(&
					(acpi_gbl_root_table_list.tables[i].
					 signature), ACPI_SIG_OSDT))
		    ||
		    ACPI_FAILURE(acpi_tb_validate_table
				 (&acpi_gbl_root_table_list.tables[i]))) {
			continue;
		}

		/* Ignore errors while loading tables, get as many as possible */

		(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
		status = acpi_ns_load_table(i, acpi_gbl_root_node);
		if (ACPI_FAILURE(status)) {
			ACPI_EXCEPTION((AE_INFO, status,
					"[%4.4s] table load failed",
					&acpi_gbl_root_table_list.tables[i].
					signature.ascii[0]));
			tables_failed++;
		} else {
			tables_loaded++;
		}

		(void)acpi_ut_acquire_mutex(ACPI_MTX_TABLES);
	}

	if (!tables_failed) {
		ACPI_INFO((AE_INFO,
			   "%u ACPI AML tables successfully acquired and loaded",
			   tables_loaded));
	} else {
		ACPI_ERROR((AE_INFO,
			    "%u ACPI AML tables successfully acquired and loaded, %u failed",
			    tables_loaded, tables_failed));
	}

unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
	return_ACPI_STATUS(status);
}
