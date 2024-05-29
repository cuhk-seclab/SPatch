int
archive_read_support_filter_rpm(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;

	return __archive_read_register_bidder(a, NULL, "rpm",
			&rpm_bidder_vtable);
}
