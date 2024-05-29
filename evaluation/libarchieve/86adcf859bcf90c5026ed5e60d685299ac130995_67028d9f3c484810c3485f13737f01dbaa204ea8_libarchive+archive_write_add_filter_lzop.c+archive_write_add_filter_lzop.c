int
archive_write_add_filter_lzop(struct archive *_a)
{
	struct archive_write_filter *f = __archive_write_allocate_filter(_a);
	struct write_lzop *data;

	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_add_filter_lzop");

	data = calloc(1, sizeof(*data));
	if (data == NULL) {
		archive_set_error(_a, ENOMEM, "Can't allocate memory");
		return (ARCHIVE_FATAL);
	}

	f->name = "lzop";
	f->code = ARCHIVE_FILTER_LZOP;
	f->data = data;
	f->open = archive_write_lzop_open;
	f->options = archive_write_lzop_options;
	f->write = archive_write_lzop_write;
	f->close = archive_write_lzop_close;
	f->free = archive_write_lzop_free;
#if defined(HAVE_LZO_LZOCONF_H) && defined(HAVE_LZO_LZO1X_H)
	if (lzo_init() != LZO_E_OK) {
		free(data);
		archive_set_error(_a, ARCHIVE_ERRNO_MISC,
		    "lzo_init(type check) failed");
		return (ARCHIVE_FATAL);
	}
	if (lzo_version() < 0x940) {
		free(data);
		archive_set_error(_a, ARCHIVE_ERRNO_MISC,
		    "liblzo library is too old(%s < 0.940)",
		    lzo_version_string());
		return (ARCHIVE_FATAL);
	}
	data->compression_level = 5;
	return (ARCHIVE_OK);
#else
	data->pdata = __archive_write_program_allocate("lzop");
	if (data->pdata == NULL) {
		free(data);
		archive_set_error(_a, ENOMEM, "Can't allocate memory");
		return (ARCHIVE_FATAL);
	}
	data->compression_level = 0;
	/* Note: We return "warn" to inform of using an external lzop
	 * program. */
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lzop program for lzop compression");
	return (ARCHIVE_WARN);
#endif
}
