static int
__archive_write_open_filter(struct archive_write_filter *f)
{
	int ret;

	ret = ARCHIVE_OK;
	if (f->next_filter != NULL)
		ret = __archive_write_open_filter(f->next_filter);
	if (ret != ARCHIVE_OK)
		return (ret);
	if (f->state != ARCHIVE_WRITE_FILTER_STATE_NEW)
		return (ARCHIVE_FATAL);
	if (f->open == NULL) {
		f->state = ARCHIVE_WRITE_FILTER_STATE_OPEN;
		return (ARCHIVE_OK);
	}
	ret = (f->open)(f);
	if (ret == ARCHIVE_OK)
		f->state = ARCHIVE_WRITE_FILTER_STATE_OPEN;
	else
		f->state = ARCHIVE_WRITE_FILTER_STATE_FATAL;
	return (ret);
}
