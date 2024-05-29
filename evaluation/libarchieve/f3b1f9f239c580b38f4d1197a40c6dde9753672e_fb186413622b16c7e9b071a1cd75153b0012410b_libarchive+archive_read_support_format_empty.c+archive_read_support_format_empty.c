int
archive_read_support_format_empty(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	int r;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_format_empty");

	r = __archive_read_register_format(a,
	    NULL,
	    "empty",
	    archive_read_format_empty_bid,
	    NULL,
	    archive_read_format_empty_read_header,
	    archive_read_format_empty_read_data,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    NULL);

	return (r);
}
