int
__archive_write_program_free(struct archive_write_program_data *data)
{

	if (data) {
#if defined(_WIN32) && !defined(__CYGWIN__)
		if (data->child)
			CloseHandle(data->child);
#endif
		free(data->program_name);
		free(data->child_buf);
		free(data);
	}
	return (ARCHIVE_OK);
}
