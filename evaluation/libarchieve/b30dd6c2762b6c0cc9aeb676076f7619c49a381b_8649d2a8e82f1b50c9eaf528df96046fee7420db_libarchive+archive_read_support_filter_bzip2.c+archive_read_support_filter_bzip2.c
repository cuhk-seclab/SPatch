int
archive_read_support_filter_bzip2(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct archive_read_filter_bidder *reader;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_filter_bzip2");

	if (__archive_read_get_bidder(a, &reader) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	reader->data = NULL;
	reader->name = "bzip2";
	reader->bid = bzip2_reader_bid;
	reader->init = bzip2_reader_init;
	reader->free = NULL;
#if defined(HAVE_BZLIB_H) && defined(BZ_CONFIG_ERROR)
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external bzip2 program");
	return (ARCHIVE_WARN);
#endif
}
