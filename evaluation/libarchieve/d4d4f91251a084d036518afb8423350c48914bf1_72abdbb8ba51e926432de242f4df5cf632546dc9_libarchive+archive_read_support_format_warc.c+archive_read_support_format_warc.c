int
archive_read_support_format_warc(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct warc_s *w;
	int r;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_format_warc");

	if ((w = calloc(1, sizeof(*w))) == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate warc data");
		return (ARCHIVE_FATAL);
	}

	r = __archive_read_register_format(
		a, w, "warc",
		_warc_bid, NULL, _warc_rdhdr, _warc_read,
		_warc_skip, NULL, _warc_cleanup, NULL, NULL);

	if (r != ARCHIVE_OK) {
		free(w);
		return (r);
	}
	return (ARCHIVE_OK);
}
