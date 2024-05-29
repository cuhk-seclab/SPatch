int
archive_read_support_filter_lzop(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct archive_read_filter_bidder *reader;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_filter_lzop");

	if (__archive_read_get_bidder(a, &reader) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	reader->data = NULL;
	reader->vtable = &lzop_bidder_vtable;
	/* Signal the extent of lzop support with the return value here. */
#if defined(HAVE_LZO_LZOCONF_H) && defined(HAVE_LZO_LZO1X_H)
	return (ARCHIVE_OK);
#else
	/* Return ARCHIVE_WARN since this always uses an external program. */
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lzop program for lzop decompression");
	return (ARCHIVE_WARN);
#endif
}
