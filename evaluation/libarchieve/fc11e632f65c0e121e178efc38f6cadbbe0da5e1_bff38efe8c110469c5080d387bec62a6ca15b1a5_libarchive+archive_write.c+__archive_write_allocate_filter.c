struct archive_write_filter *
__archive_write_allocate_filter(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct archive_write_filter *f;

	f = calloc(1, sizeof(*f));

	if (f == NULL)
		return (NULL);

	f->archive = _a;
	f->state = ARCHIVE_WRITE_FILTER_STATE_NEW;
	if (a->filter_first == NULL)
		a->filter_first = f;
	else
		a->filter_last->next_filter = f;
	a->filter_last = f;
	return f;
}
