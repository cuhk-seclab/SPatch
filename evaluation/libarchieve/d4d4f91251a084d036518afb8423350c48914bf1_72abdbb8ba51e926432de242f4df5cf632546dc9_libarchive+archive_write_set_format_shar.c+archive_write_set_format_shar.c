int
archive_write_set_format_shar(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct shar *shar;

	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_set_format_shar");

	/* If someone else was already registered, unregister them. */
	if (a->format_free != NULL)
		(a->format_free)(a);

	shar = (struct shar *)calloc(1, sizeof(*shar));
	if (shar == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate shar data");
		return (ARCHIVE_FATAL);
	}
	archive_string_init(&shar->work);
	archive_string_init(&shar->quoted_name);
	a->format_data = shar;
	a->format_name = "shar";
	a->format_write_header = archive_write_shar_header;
	a->format_close = archive_write_shar_close;
	a->format_free = archive_write_shar_free;
	a->format_write_data = archive_write_shar_data_sed;
	a->format_finish_entry = archive_write_shar_finish_entry;
	a->archive.archive_format = ARCHIVE_FORMAT_SHAR_BASE;
	a->archive.archive_format_name = "shar";
	return (ARCHIVE_OK);
}
