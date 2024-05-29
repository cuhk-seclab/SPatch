int
archive_read_support_filter_bzip2(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (__archive_read_register_bidder(a, NULL, "bzip2",
				&bzip2_bidder_vtable) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

#if defined(HAVE_BZLIB_H) && defined(BZ_CONFIG_ERROR)
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external bzip2 program");
	return (ARCHIVE_WARN);
#endif
}
