int
archive_read_support_filter_zstd(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (__archive_read_register_bidder(a, NULL, "zstd",
				&zstd_bidder_vtable) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

#if HAVE_ZSTD_H && HAVE_LIBZSTD
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external zstd program for zstd decompression");
	return (ARCHIVE_WARN);
#endif
}
