int
archive_read_add_callback_data(struct archive *_a, void *client_data,
    unsigned int iindex)
{
	struct archive_read *a = (struct archive_read *)_a;
	void *p;
	unsigned int i;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW,
	    "archive_read_add_callback_data");
	if (iindex > a->client.nodes) {
		archive_set_error(&a->archive, EINVAL,
			"Invalid index specified.");
		return ARCHIVE_FATAL;
	}
	p = realloc(a->client.dataset, sizeof(*a->client.dataset)
		* (++(a->client.nodes)));
	if (p == NULL) {
		archive_set_error(&a->archive, ENOMEM,
			"No memory.");
		return ARCHIVE_FATAL;
	}
	a->client.dataset = (struct archive_read_data_node *)p;
	for (i = a->client.nodes - 1; i > iindex && i > 0; i--) {
		a->client.dataset[i].data = a->client.dataset[i-1].data;
		a->client.dataset[i].begin_position = -1;
		a->client.dataset[i].total_size = -1;
	}
	a->client.dataset[iindex].data = client_data;
	a->client.dataset[iindex].begin_position = -1;
	a->client.dataset[iindex].total_size = -1;
	return ARCHIVE_OK;
}
