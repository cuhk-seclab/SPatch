int
archive_read_support_filter_lzma(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (__archive_read_register_bidder(a, NULL, "lzma",
				&lzma_bidder_vtable) != ARCHIVE_OK)
		return (ARCHIVE_FATAL);

#if HAVE_LZMA_H && HAVE_LIBLZMA
	return (ARCHIVE_OK);
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC,
	    "Using external lzma program for lzma decompression");
	return (ARCHIVE_WARN);
#endif
}
