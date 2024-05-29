acpi_status
acpi_tb_install_standard_table(acpi_physical_address address,
			       u8 flags,
			       u8 reload, u8 override, u32 *table_index)
{
	u32 i;
	acpi_status status = AE_OK;
	struct acpi_table_desc new_table_desc;

	ACPI_FUNCTION_TRACE(tb_install_standard_table);

	/* Acquire a temporary table descriptor for validation */

	status = acpi_tb_acquire_temp_table(&new_table_desc, address, flags);
	if (ACPI_FAILURE(status)) {
		ACPI_ERROR((AE_INFO,
			    "Could not acquire table length at %8.8X%8.8X",
			    ACPI_FORMAT_UINT64(address)));
		return_ACPI_STATUS(status);
	}

	/*
	 * Optionally do not load any SSDTs from the RSDT/XSDT. This can
	 * be useful for debugging ACPI problems on some machines.
	 */
	if (!reload &&
	    acpi_gbl_disable_ssdt_table_install &&
	    ACPI_COMPARE_NAME(&new_table_desc.signature, ACPI_SIG_SSDT)) {
		ACPI_INFO((AE_INFO,
			   "Ignoring installation of %4.4s at %8.8X%8.8X",
			   new_table_desc.signature.ascii,
			   ACPI_FORMAT_UINT64(address)));
		goto release_and_exit;
	}

	/* Validate and verify a table before installation */

	status = acpi_tb_verify_temp_table(&new_table_desc, NULL);
	if (ACPI_FAILURE(status)) {
		goto release_and_exit;
	}

	if (reload) {
		/*
		 * Validate the incoming table signature.
		 *
		 * 1) Originally, we checked the table signature for "SSDT" or "PSDT".
		 * 2) We added support for OEMx tables, signature "OEM".
		 * 3) Valid tables were encountered with a null signature, so we just
		 *    gave up on validating the signature, (05/2008).
		 * 4) We encountered non-AML tables such as the MADT, which caused
		 *    interpreter errors and kernel faults. So now, we once again allow
		 *    only "SSDT", "OEMx", and now, also a null signature. (05/2011).
		 */
		if ((new_table_desc.signature.ascii[0] != 0x00) &&
		    (!ACPI_COMPARE_NAME
		     (&new_table_desc.signature, ACPI_SIG_SSDT))
		    && (ACPI_STRNCMP(new_table_desc.signature.ascii, "OEM", 3)))
		{
			ACPI_BIOS_ERROR((AE_INFO,
					 "Table has invalid signature [%4.4s] (0x%8.8X), "
					 "must be SSDT or OEMx",
					 acpi_ut_valid_acpi_name(new_table_desc.
								 signature.
								 ascii) ?
					 new_table_desc.signature.
					 ascii : "????",
					 new_table_desc.signature.integer));

			status = AE_BAD_SIGNATURE;
			goto release_and_exit;
		}

		/* Check if table is already registered */

		for (i = 0; i < acpi_gbl_root_table_list.current_table_count;
		     ++i) {
			/*
			 * Check for a table match on the entire table length,
			 * not just the header.
			 */
			if (!acpi_tb_compare_tables(&new_table_desc, i)) {
				continue;
			}

			/*
			 * Note: the current mechanism does not unregister a table if it is
			 * dynamically unloaded. The related namespace entries are deleted,
			 * but the table remains in the root table list.
			 *
			 * The assumption here is that the number of different tables that
			 * will be loaded is actually small, and there is minimal overhead
			 * in just keeping the table in case it is needed again.
			 *
			 * If this assumption changes in the future (perhaps on large
			 * machines with many table load/unload operations), tables will
			 * need to be unregistered when they are unloaded, and slots in the
			 * root table list should be reused when empty.
			 */
			if (acpi_gbl_root_table_list.tables[i].
			    flags & ACPI_TABLE_IS_LOADED) {

				/* Table is still loaded, this is an error */

				status = AE_ALREADY_EXISTS;
				goto release_and_exit;
			} else {
				/*
				 * Table was unloaded, allow it to be reloaded.
				 * As we are going to return AE_OK to the caller, we should
				 * take the responsibility of freeing the input descriptor.
				 * Refill the input descriptor to ensure
				 * acpi_tb_install_table_with_override() can be called again to
				 * indicate the re-installation.
				 */
				acpi_tb_uninstall_table(&new_table_desc);
				*table_index = i;
				(void)acpi_ut_release_mutex(ACPI_MTX_TABLES);
				return_ACPI_STATUS(AE_OK);
			}
		}
	}

	/* Add the table to the global root table list */

	status = acpi_tb_get_next_root_index(&i);
	if (ACPI_FAILURE(status)) {
		goto release_and_exit;
	}

	*table_index = i;
	acpi_tb_install_table_with_override(i, &new_table_desc, override);

release_and_exit:

	/* Release the temporary table descriptor */

	acpi_tb_release_temp_table(&new_table_desc);
	return_ACPI_STATUS(status);
}
