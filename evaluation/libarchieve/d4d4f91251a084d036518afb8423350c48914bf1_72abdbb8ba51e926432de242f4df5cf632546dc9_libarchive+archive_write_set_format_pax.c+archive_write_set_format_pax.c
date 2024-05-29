int
archive_write_set_format_pax(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct pax *pax;

	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_set_format_pax");

	if (a->format_free != NULL)
		(a->format_free)(a);

	pax = (struct pax *)calloc(1, sizeof(*pax));
	if (pax == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate pax data");
		return (ARCHIVE_FATAL);
	}
	a->format_data = pax;
	a->format_name = "pax";
	a->format_options = archive_write_pax_options;
	a->format_write_header = archive_write_pax_header;
	a->format_write_data = archive_write_pax_data;
	a->format_close = archive_write_pax_close;
	a->format_free = archive_write_pax_free;
	a->format_finish_entry = archive_write_pax_finish_entry;
	a->archive.archive_format = ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE;
	a->archive.archive_format_name = "POSIX pax interchange";
	return (ARCHIVE_OK);
}
