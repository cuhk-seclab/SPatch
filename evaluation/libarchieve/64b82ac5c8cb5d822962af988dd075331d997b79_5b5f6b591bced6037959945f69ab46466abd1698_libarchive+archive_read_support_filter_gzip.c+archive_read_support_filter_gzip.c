int
archive_read_support_filter_gzip(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (__archive_read_register_bidder(a, NULL, "gzip",
				&gzip_bidder_vtable) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

	/* Signal the extent of gzip support with the return value here. */
#if HAVE_ZLIB_H
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external gzip program");
	return (ARCHIVE_WARN);
#endif
}
