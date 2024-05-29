int
archive_write_open(struct archive *_a, void *client_data,
    archive_open_callback *opener, archive_write_callback *writer,
    archive_close_callback *closer)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct archive_write_filter *client_filter;
	int ret, r1;

	archive_check_magic(&a->archive, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_open");
	archive_clear_error(&a->archive);

	a->client_writer = writer;
	a->client_opener = opener;
	a->client_closer = closer;
	a->client_data = client_data;

	client_filter = __archive_write_allocate_filter(_a);
	client_filter->open = archive_write_client_open;
	client_filter->write = archive_write_client_write;
	client_filter->close = archive_write_client_close;

	ret = __archive_write_open_filter(a->filter_first);
	if (ret < ARCHIVE_WARN) {
		r1 = __archive_write_close_filter(a);
		__archive_write_filters_free(_a);
		return (r1 < ret ? r1 : ret);
	}

	a->archive.state = ARCHIVE_STATE_HEADER;
	if (a->format_init)
		ret = (a->format_init)(a);
	return (ret);
}
