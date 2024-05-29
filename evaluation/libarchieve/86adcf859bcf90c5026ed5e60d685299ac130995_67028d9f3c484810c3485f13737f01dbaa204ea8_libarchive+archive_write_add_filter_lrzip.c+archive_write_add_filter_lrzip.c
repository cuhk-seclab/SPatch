int
archive_write_add_filter_lrzip(struct archive *_a)
{
	struct archive_write_filter *f = __archive_write_allocate_filter(_a);
	struct write_lrzip *data;

	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_add_filter_lrzip");

	data = calloc(1, sizeof(*data));
	if (data == NULL) {
		archive_set_error(_a, ENOMEM, "Can't allocate memory");
		return (ARCHIVE_FATAL);
	}
	data->pdata = __archive_write_program_allocate("lrzip");
	if (data->pdata == NULL) {
		free(data);
		archive_set_error(_a, ENOMEM, "Can't allocate memory");
		return (ARCHIVE_FATAL);
	}

	f->name = "lrzip";
	f->code = ARCHIVE_FILTER_LRZIP;
	f->data = data;
	f->open = archive_write_lrzip_open;
	f->options = archive_write_lrzip_options;
	f->write = archive_write_lrzip_write;
	f->close = archive_write_lrzip_close;
	f->free = archive_write_lrzip_free;

	/* Note: This filter always uses an external program, so we
	 * return "warn" to inform of the fact. */
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lrzip program for lrzip compression");
	return (ARCHIVE_WARN);
}
