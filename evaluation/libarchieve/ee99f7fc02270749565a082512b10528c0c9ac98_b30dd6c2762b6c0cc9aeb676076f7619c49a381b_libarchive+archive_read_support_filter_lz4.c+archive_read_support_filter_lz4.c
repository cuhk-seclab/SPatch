int
archive_read_support_filter_lz4(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct archive_read_filter_bidder *reader;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_filter_lz4");

	if (__archive_read_get_bidder(a, &reader) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	reader->data = NULL;
	reader->name = "lz4";
	reader->bid = lz4_reader_bid;
	reader->init = lz4_reader_init;
	reader->free = lz4_reader_free;
#if defined(HAVE_LIBLZ4)
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lz4 program");
	return (ARCHIVE_WARN);
#endif
}
