int
archive_read_support_filter_zstd(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct archive_read_filter_bidder *bidder;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_filter_zstd");

	if (__archive_read_get_bidder(a, &bidder) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	bidder->data = NULL;
	bidder->name = "zstd";
	bidder->bid = zstd_bidder_bid;
	bidder->init = zstd_bidder_init;
	bidder->options = NULL;
	bidder->free = NULL;
#if HAVE_ZSTD_H && HAVE_LIBZSTD
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external zstd program for zstd decompression");
	return (ARCHIVE_WARN);
#endif
}
