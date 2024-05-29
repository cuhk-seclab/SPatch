int
archive_read_support_format_ar(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct ar *ar;
	int r;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_format_ar");

	ar = (struct ar *)calloc(1, sizeof(*ar));
	if (ar == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate ar data");
		return (ARCHIVE_FATAL);
	}
	ar->strtab = NULL;

	r = __archive_read_register_format(a,
	    ar,
	    "ar",
	    archive_read_format_ar_bid,
	    NULL,
	    archive_read_format_ar_read_header,
	    archive_read_format_ar_read_data,
	    archive_read_format_ar_skip,
	    NULL,
	    archive_read_format_ar_cleanup,
	    NULL,
	    NULL);

	if (r != ARCHIVE_OK) {
		free(ar);
		return (r);
	}
	return (ARCHIVE_OK);
}
