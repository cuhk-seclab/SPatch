int
archive_write_add_filter_program(struct archive *_a, const char *cmd)
{
	struct archive_write_filter *f = __archive_write_allocate_filter(_a);
	struct private_data *data;
	static const char prefix[] = "Program: ";

	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_add_filter_program");

	f->data = calloc(1, sizeof(*data));
	if (f->data == NULL)
		goto memerr;
	data = (struct private_data *)f->data;

	data->cmd = strdup(cmd);
	if (data->cmd == NULL)
		goto memerr;

	data->pdata = __archive_write_program_allocate(cmd);
	if (data->pdata == NULL)
		goto memerr;

	/* Make up a description string. */
	if (archive_string_ensure(&data->description,
	    strlen(prefix) + strlen(cmd) + 1) == NULL)
		goto memerr;
	archive_strcpy(&data->description, prefix);
	archive_strcat(&data->description, cmd);

	f->name = data->description.s;
	f->code = ARCHIVE_FILTER_PROGRAM;
	f->open = archive_compressor_program_open;
	f->write = archive_compressor_program_write;
	f->close = archive_compressor_program_close;
	f->free = archive_compressor_program_free;
	return (ARCHIVE_OK);
memerr:
	archive_compressor_program_free(f);
	archive_set_error(_a, ENOMEM,
	    "Can't allocate memory for filter program");
	return (ARCHIVE_FATAL);
}
