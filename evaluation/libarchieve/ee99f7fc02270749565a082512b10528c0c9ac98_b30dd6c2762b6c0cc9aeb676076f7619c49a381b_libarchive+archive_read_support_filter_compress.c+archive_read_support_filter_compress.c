int
archive_read_support_filter_compress(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct archive_read_filter_bidder *bidder;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_filter_compress");

	if (__archive_read_get_bidder(a, &bidder) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	bidder->data = NULL;
	bidder->name = "compress (.Z)";
	bidder->bid = compress_bidder_bid;
	bidder->init = compress_bidder_init;
	bidder->free = compress_bidder_free;
	return (ARCHIVE_OK);
}
