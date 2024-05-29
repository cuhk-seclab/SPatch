la_ssize_t
archive_read_data(struct archive *_a, void *buff, size_t s)
{
	struct archive *a = (struct archive *)_a;
	char	*dest;
	const void *read_buf;
	size_t	 bytes_read;
	size_t	 len;
	int	 r;

	bytes_read = 0;
	dest = (char *)buff;

	while (s > 0) {
		if (a->read_data_offset == a->read_data_output_offset &&
		    a->read_data_remaining == 0) {
			read_buf = a->read_data_block;
			a->read_data_is_posix_read = 1;
			a->read_data_requested = s;
			r = archive_read_data_block(a, &read_buf,
			    &a->read_data_remaining, &a->read_data_offset);
			a->read_data_block = read_buf;
			if (r == ARCHIVE_EOF)
				return (bytes_read);
			/*
			 * Error codes are all negative, so the status
			 * return here cannot be confused with a valid
			 * byte count.  (ARCHIVE_OK is zero.)
			 */
			if (r < ARCHIVE_OK)
				return (r);
		}

		if (a->read_data_offset < a->read_data_output_offset) {
			archive_set_error(a, ARCHIVE_ERRNO_FILE_FORMAT,
			    "Encountered out-of-order sparse blocks");
			return (ARCHIVE_RETRY);
		}

		/* Compute the amount of zero padding needed. */
		if (a->read_data_output_offset + (int64_t)s <
		    a->read_data_offset) {
			len = s;
		} else if (a->read_data_output_offset <
		    a->read_data_offset) {
			len = (size_t)(a->read_data_offset -
			    a->read_data_output_offset);
		} else
			len = 0;

		/* Add zeroes. */
		memset(dest, 0, len);
		s -= len;
		a->read_data_output_offset += len;
		dest += len;
		bytes_read += len;

		/* Copy data if there is any space left. */
		if (s > 0) {
			len = a->read_data_remaining;
			if (len > s)
				len = s;
			if (len)
				memcpy(dest, a->read_data_block, len);
				a->read_data_block += len;
				a->read_data_remaining -= len;
				a->read_data_output_offset += len;
				a->read_data_offset += len;
				dest += len;
				bytes_read += len;
			s -= len;
			bytes_read += len;
		}
	}
	a->read_data_is_posix_read = 0;
	a->read_data_requested = 0;
	return (bytes_read);
}
