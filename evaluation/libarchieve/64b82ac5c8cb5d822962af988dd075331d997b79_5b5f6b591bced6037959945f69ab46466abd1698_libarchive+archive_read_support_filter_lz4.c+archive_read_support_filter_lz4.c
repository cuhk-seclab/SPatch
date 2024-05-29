int
archive_read_support_filter_lz4(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (__archive_read_register_bidder(a, NULL, "lz4",
				&lz4_bidder_vtable) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

#if defined(HAVE_LIBLZ4)
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lz4 program");
	return (ARCHIVE_WARN);
#endif
}
