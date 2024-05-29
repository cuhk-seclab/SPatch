int
archive_write_add_filter_bzip2(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct archive_write_filter *f = __archive_write_allocate_filter(_a);
	struct private_data *data;

	archive_check_magic(&a->archive, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_add_filter_bzip2");

	data = calloc(1, sizeof(*data));
	if (data == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Out of memory");
		return (ARCHIVE_FATAL);
	}
	data->compression_level = 9; /* default */

	f->data = data;
	f->options = &archive_compressor_bzip2_options;
	f->close = &archive_compressor_bzip2_close;
	f->free = &archive_compressor_bzip2_free;
	f->open = &archive_compressor_bzip2_open;
	f->code = ARCHIVE_FILTER_BZIP2;
	f->name = "bzip2";
#if defined(HAVE_BZLIB_H) && defined(BZ_CONFIG_ERROR)
	return (ARCHIVE_OK);
#else
	data->pdata = __archive_write_program_allocate("bzip2");
	if (data->pdata == NULL) {
		free(data);
		archive_set_error(&a->archive, ENOMEM, "Out of memory");
		return (ARCHIVE_FATAL);
	}
	data->compression_level = 0;
	archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
	    "Using external bzip2 program");
	return (ARCHIVE_WARN);
#endif
}
