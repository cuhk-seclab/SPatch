void
__archive_read_free_filters(struct archive_read *a)
{
	/* Make sure filters are closed and their buffers are freed */
	close_filters(a);

	while (a->filter != NULL) {
		struct archive_read_filter *t = a->filter->upstream;
		free(a->filter);
		a->filter = t;
	}
}
