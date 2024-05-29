int
archive_write_open_fd(struct archive *a, int fd)
{
	struct write_fd_data *mine;

	mine = (struct write_fd_data *)malloc(sizeof(*mine));
	if (mine == NULL) {
		archive_set_error(a, ENOMEM, "No memory");
		return (ARCHIVE_FATAL);
	}
	mine->fd = fd;
#if defined(__CYGWIN__) || defined(_WIN32)
	setmode(mine->fd, O_BINARY);
#endif
	return (archive_write_open2(a, mine,
		    file_open, file_write, NULL, file_free));
}
