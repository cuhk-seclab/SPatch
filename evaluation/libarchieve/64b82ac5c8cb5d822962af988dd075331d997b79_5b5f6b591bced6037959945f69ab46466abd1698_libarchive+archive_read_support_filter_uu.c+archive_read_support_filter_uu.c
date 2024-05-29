int
archive_read_support_filter_uu(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	return __archive_read_register_bidder(a, NULL, "uu",
			&uudecode_bidder_vtable);
}
