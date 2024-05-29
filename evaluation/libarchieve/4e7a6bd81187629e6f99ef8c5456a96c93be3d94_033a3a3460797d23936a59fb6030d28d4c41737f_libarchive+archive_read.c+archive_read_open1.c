int
archive_read_open1(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct archive_read_filter *filter, *tmp;
	int slot, e = ARCHIVE_OK;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW,
	    "archive_read_open");
	archive_clear_error(&a->archive);

	if (a->client.reader == NULL) {
		archive_set_error(&a->archive, EINVAL,
		    "No reader function provided to archive_read_open");
		a->archive.state = ARCHIVE_STATE_FATAL;
		return (ARCHIVE_FATAL);
	}

	/* Open data source. */
	if (a->client.opener != NULL) {
		e = (a->client.opener)(&a->archive, a->client.dataset[0].data);
		if (e != 0) {
			/* If the open failed, call the closer to clean up. */
			read_client_close_proxy(a);
			return (e);
		}
	}

	filter = calloc(1, sizeof(*filter));
	if (filter == NULL)
		return (ARCHIVE_FATAL);
	filter->bidder = NULL;
	filter->upstream = NULL;
	filter->archive = a;
	filter->data = a->client.dataset[0].data;
	filter->open = client_open_proxy;
	filter->read = client_read_proxy;
	filter->close = client_close_proxy;
	filter->sswitch = client_switch_proxy;
	filter->name = "none";
	filter->code = ARCHIVE_FILTER_NONE;
	filter->can_skip = 1;
	filter->can_seek = 1;

	a->client.dataset[0].begin_position = 0;
	if (!a->filter || !a->bypass_filter_bidding)
	{
		a->filter = filter;
		/* Build out the input pipeline. */
		e = choose_filters(a);
		if (e < ARCHIVE_WARN) {
			a->archive.state = ARCHIVE_STATE_FATAL;
			return (ARCHIVE_FATAL);
		}
	}
	else
	{
		/* Need to add "NONE" type filter at the end of the filter chain */
		tmp = a->filter;
		while (tmp->upstream)
			tmp = tmp->upstream;
		tmp->upstream = filter;
	}

	if (!a->format)
	{
		slot = choose_format(a);
		if (slot < 0) {
			close_filters(a);
			a->archive.state = ARCHIVE_STATE_FATAL;
			return (ARCHIVE_FATAL);
		}
		a->format = &(a->formats[slot]);
	}

	a->archive.state = ARCHIVE_STATE_HEADER;

	/* Ensure libarchive starts from the first node in a multivolume set */
	client_switch_proxy(a->filter, 0);
	return (e);
}
