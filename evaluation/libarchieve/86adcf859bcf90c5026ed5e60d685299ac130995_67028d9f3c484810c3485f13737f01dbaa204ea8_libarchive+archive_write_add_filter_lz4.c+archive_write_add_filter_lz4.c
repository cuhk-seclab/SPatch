int
archive_write_add_filter_lz4(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct archive_write_filter *f = __archive_write_allocate_filter(_a);
	struct private_data *data;

	archive_check_magic(&a->archive, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_add_filter_lz4");

	data = calloc(1, sizeof(*data));
	if (data == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Out of memory");
		return (ARCHIVE_FATAL);
	}

	/*
	 * Setup default settings.
	 */
	data->compression_level = 1;
	data->version_number = 0x01;
	data->block_independence = 1;
	data->block_checksum = 0;
	data->stream_size = 0;
	data->stream_checksum = 1;
	data->preset_dictionary = 0;
	data->block_maximum_size = 7;

	/*
	 * Setup a filter setting.
	 */
	f->data = data;
	f->options = &archive_filter_lz4_options;
	f->close = &archive_filter_lz4_close;
	f->free = &archive_filter_lz4_free;
	f->open = &archive_filter_lz4_open;
	f->code = ARCHIVE_FILTER_LZ4;
	f->name = "lz4";
#if defined(HAVE_LIBLZ4) && LZ4_VERSION_MAJOR >= 1 && LZ4_VERSION_MINOR >= 2
	return (ARCHIVE_OK);
#else
	/*
	 * We don't have lz4 library, and execute external lz4 program
	 * instead.
	 */
	data->pdata = __archive_write_program_allocate("lz4");
	if (data->pdata == NULL) {
		free(data);
		archive_set_error(&a->archive, ENOMEM, "Out of memory");
		return (ARCHIVE_FATAL);
	}
	data->compression_level = 0;
	archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
	    "Using external lz4 program");
	return (ARCHIVE_WARN);
#endif
}
