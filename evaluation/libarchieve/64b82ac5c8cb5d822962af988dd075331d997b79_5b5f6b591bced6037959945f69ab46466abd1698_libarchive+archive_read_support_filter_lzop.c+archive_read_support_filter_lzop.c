int
archive_read_support_filter_lzop(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (__archive_read_register_bidder(a, NULL, NULL,
				&lzop_bidder_vtable) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

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
