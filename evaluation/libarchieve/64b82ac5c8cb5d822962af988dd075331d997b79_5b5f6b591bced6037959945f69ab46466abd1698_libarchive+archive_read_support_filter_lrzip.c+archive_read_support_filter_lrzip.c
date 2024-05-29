int
archive_read_support_filter_lrzip(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (__archive_read_register_bidder(a, NULL, "lrzip",
				&lrzip_bidder_vtable) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	/* This filter always uses an external program. */
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lrzip program for lrzip decompression");
	return (ARCHIVE_WARN);
}
