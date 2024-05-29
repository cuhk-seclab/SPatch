int
__archive_write_program_open(struct archive_write_filter *f,
    struct archive_write_program_data *data, const char *cmd)
{
	int ret;

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

	ret = __archive_create_child(cmd, &data->child_stdin,
		    &data->child_stdout, &data->child);
	if (ret != ARCHIVE_OK) {
		archive_set_error(f->archive, EINVAL,
		    "Can't launch external program: %s", cmd);
		return (ARCHIVE_FATAL);
	}
	return (ARCHIVE_OK);
}
