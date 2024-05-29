acpi_status
acpi_ds_method_error(acpi_status status, struct acpi_walk_state * walk_state)
{
	u32 aml_offset;

	ACPI_FUNCTION_ENTRY();

	/* Ignore AE_OK and control exception codes */

	if (ACPI_SUCCESS(status) || (status & AE_CODE_CONTROL)) {
		return (status);
	}

	/* Invoke the global exception handler */

	if (acpi_gbl_exception_handler) {

		/* Exit the interpreter, allow handler to execute methods */

		acpi_ex_exit_interpreter();

		/*
		 * Handler can map the exception code to anything it wants, including
		 * AE_OK, in which case the executing method will not be aborted.
		 */
		aml_offset = (u32)ACPI_PTR_DIFF(walk_state->aml,
						walk_state->parser_state.
						aml_start);

		status = acpi_gbl_exception_handler(status,
						    walk_state->method_node ?
						    walk_state->method_node->
						    name.integer : 0,
						    walk_state->opcode,
						    aml_offset, NULL);
		acpi_ex_enter_interpreter();
	}

	acpi_ds_clear_implicit_return(walk_state);

	if (ACPI_FAILURE(status)) {
		acpi_ds_dump_method_stack(status, walk_state, walk_state->op);

		/* Display method locals/args if debugger is present */

#ifdef ACPI_DEBUGGER
		acpi_db_dump_method_info(status, walk_state);
#endif
	}

	return (status);
}
