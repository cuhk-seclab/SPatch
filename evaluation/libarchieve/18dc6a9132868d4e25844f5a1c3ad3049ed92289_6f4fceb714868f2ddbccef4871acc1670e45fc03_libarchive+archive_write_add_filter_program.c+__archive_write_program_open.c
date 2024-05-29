int
__archive_write_program_open(struct archive_write_filter *f,
    struct archive_write_program_data *data, const char *cmd)
{
	pid_t child;

	if (data->child_buf == NULL) {
		data->child_buf_len = 65536;
		data->child_buf_avail = 0;
		data->child_buf = malloc(data->child_buf_len);

		if (data->child_buf == NULL) {
			archive_set_error(f->archive, ENOMEM,
			    "Can't allocate compression buffer");
			return (ARCHIVE_FATAL);
		}
	}

	child = __archive_create_child(cmd, &data->child_stdin,
		    &data->child_stdout);
	if (child == -1) {
		archive_set_error(f->archive, EINVAL,
		    "Can't launch external program: %s", cmd);
		return (ARCHIVE_FATAL);
	}
#if defined(_WIN32) && !defined(__CYGWIN__)
	data->child = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, child);
	if (data->child == NULL) {
		close(data->child_stdin);
		data->child_stdin = -1;
		close(data->child_stdout);
		data->child_stdout = -1;
		archive_set_error(f->archive, EINVAL,
		    "Can't launch external program: %s", cmd);
		return (ARCHIVE_FATAL);
	}
#else
	data->child = child;
#endif
	return (ARCHIVE_OK);
}
