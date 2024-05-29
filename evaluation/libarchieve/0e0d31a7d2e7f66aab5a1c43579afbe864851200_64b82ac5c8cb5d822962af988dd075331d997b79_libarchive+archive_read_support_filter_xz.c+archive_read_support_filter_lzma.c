int
archive_read_support_filter_lzma(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct archive_read_filter_bidder *bidder;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_filter_lzma");

	if (__archive_read_get_bidder(a, &bidder) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	bidder->data = NULL;
	bidder->name = "lzma";
	bidder->vtable = &lzma_bidder_vtable;
#if HAVE_LZMA_H && HAVE_LIBLZMA
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lzma program for lzma decompression");
	return (ARCHIVE_WARN);
#endif
}
