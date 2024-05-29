int
archive_read_data_into_fd(struct archive *a, int fd)
{
	struct stat st;
	int r, r2;
	const void *buff;
	size_t size, bytes_to_write;
	ssize_t bytes_written;
	int64_t target_offset;
	int64_t actual_offset = 0;
	int can_lseek;
	char *nulls = NULL;
	size_t nulls_size = 16384;

	archive_check_magic(a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_DATA,
	    "archive_read_data_into_fd");

	can_lseek = (fstat(fd, &st) == 0) && S_ISREG(st.st_mode);
	if (!can_lseek) {
		nulls = calloc(1, nulls_size);
		if (!nulls) {
			r = ARCHIVE_FATAL;
			goto cleanup;
		}
	}

	while ((r = archive_read_data_block(a, &buff, &size, &target_offset)) ==
	    ARCHIVE_OK) {
		const char *p = buff;
		if (target_offset > actual_offset) {
			r = pad_to(a, fd, can_lseek, nulls_size, nulls,
			    target_offset, actual_offset);
			if (r != ARCHIVE_OK)
				break;
			actual_offset = target_offset;
		}
		while (size > 0) {
			bytes_to_write = size;
			if (bytes_to_write > MAX_WRITE)
				bytes_to_write = MAX_WRITE;
			bytes_written = write(fd, p, bytes_to_write);
			if (bytes_written < 0) {
				archive_set_error(a, errno, "Write error");
				r = ARCHIVE_FATAL;
				goto cleanup;
			}
			actual_offset += bytes_written;
			p += bytes_written;
			size -= bytes_written;
		}
	}

	if (r == ARCHIVE_EOF && target_offset > actual_offset) {
		r2 = pad_to(a, fd, can_lseek, nulls_size, nulls,
		    target_offset, actual_offset);
		if (r2 != ARCHIVE_OK)
			r = r2;
	}

cleanup:
	free(nulls);
	if (r != ARCHIVE_EOF)
		return (r);
	return (ARCHIVE_OK);
}
