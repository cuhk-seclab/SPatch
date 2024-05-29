int
archive_write_add_filter_gzip(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct archive_write_filter *f = __archive_write_allocate_filter(_a);
	struct private_data *data;
	archive_check_magic(&a->archive, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_add_filter_gzip");

	data = calloc(1, sizeof(*data));
	if (data == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Out of memory");
		return (ARCHIVE_FATAL);
	}
	f->data = data;
	f->open = &archive_compressor_gzip_open;
	f->options = &archive_compressor_gzip_options;
	f->close = &archive_compressor_gzip_close;
	f->free = &archive_compressor_gzip_free;
	f->code = ARCHIVE_FILTER_GZIP;
	f->name = "gzip";
#ifdef HAVE_ZLIB_H
	data->compression_level = Z_DEFAULT_COMPRESSION;
	return (ARCHIVE_OK);
#else
	data->pdata = __archive_write_program_allocate("gzip");
	if (data->pdata == NULL) {
		free(data);
		archive_set_error(&a->archive, ENOMEM, "Out of memory");
		return (ARCHIVE_FATAL);
	}
	data->compression_level = 0;
	archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
	    "Using external gzip program");
	return (ARCHIVE_WARN);
#endif
}
