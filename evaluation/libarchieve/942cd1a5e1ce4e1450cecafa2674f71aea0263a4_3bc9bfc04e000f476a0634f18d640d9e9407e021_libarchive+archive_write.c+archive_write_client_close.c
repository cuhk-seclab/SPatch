static int
archive_write_client_close(struct archive_write_filter *f)
{
	struct archive_write *a = (struct archive_write *)f->archive;
	struct archive_none *state = (struct archive_none *)f->data;
	ssize_t block_length;
	ssize_t target_block_length;
	ssize_t bytes_written;
	size_t to_write;
	char *p;
	int ret = ARCHIVE_OK;

	/* If there's pending data, pad and write the last block */
	if (state->next != state->buffer) {
		block_length = state->buffer_size - state->avail;

		/* Tricky calculation to determine size of last block */
		if (a->bytes_in_last_block <= 0)
			/* Default or Zero: pad to full block */
			target_block_length = a->bytes_per_block;
		else
			/* Round to next multiple of bytes_in_last_block. */
			target_block_length = a->bytes_in_last_block *
			    ( (block_length + a->bytes_in_last_block - 1) /
			        a->bytes_in_last_block);
		if (target_block_length > a->bytes_per_block)
			target_block_length = a->bytes_per_block;
		if (block_length < target_block_length) {
			memset(state->next, 0,
			    target_block_length - block_length);
			block_length = target_block_length;
		}
		p = state->buffer;
		to_write = block_length;
		while (to_write > 0) {
			bytes_written = (a->client_writer)(&a->archive,
			    a->client_data, p, to_write);
			if (bytes_written <= 0) {
				ret = ARCHIVE_FATAL;
				break;
			}
			if ((size_t)bytes_written > to_write) {
				archive_set_error(&(a->archive),
						  -1, "write overrun");
				ret = ARCHIVE_FATAL;
				break;
			}
			p += bytes_written;
			to_write -= bytes_written;
		}
	}
	if (a->client_closer)
		(*a->client_closer)(&a->archive, a->client_data);
	free(state->buffer);
	free(state);

	/* Clear the close handler myself not to be called again. */
	f->state = ARCHIVE_WRITE_FILTER_STATE_CLOSED;
	return (ret);
}
