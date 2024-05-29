int
archive_write_set_format_ustar(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct ustar *ustar;

	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_set_format_ustar");

	/* If someone else was already registered, unregister them. */
	if (a->format_free != NULL)
		(a->format_free)(a);

	/* Basic internal sanity test. */
	if (sizeof(template_header) != 512) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
		    "Internal: template_header wrong size: %zu should be 512",
		    sizeof(template_header));
		return (ARCHIVE_FATAL);
	}

	ustar = (struct ustar *)calloc(1, sizeof(*ustar));
	if (ustar == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate ustar data");
		return (ARCHIVE_FATAL);
	}
	a->format_data = ustar;
	a->format_name = "ustar";
	a->format_options = archive_write_ustar_options;
	a->format_write_header = archive_write_ustar_header;
	a->format_write_data = archive_write_ustar_data;
	a->format_close = archive_write_ustar_close;
	a->format_free = archive_write_ustar_free;
	a->format_finish_entry = archive_write_ustar_finish_entry;
	a->archive.archive_format = ARCHIVE_FORMAT_TAR_USTAR;
	a->archive.archive_format_name = "POSIX ustar";
	return (ARCHIVE_OK);
}
