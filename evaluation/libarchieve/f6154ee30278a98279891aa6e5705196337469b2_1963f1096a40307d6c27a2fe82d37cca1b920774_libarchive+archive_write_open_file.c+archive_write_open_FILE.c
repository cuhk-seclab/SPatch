int
archive_write_open_FILE(struct archive *a, FILE *f)
{
	struct write_FILE_data *mine;

	mine = (struct write_FILE_data *)malloc(sizeof(*mine));
	if (mine == NULL) {
		archive_set_error(a, ENOMEM, "No memory");
		return (ARCHIVE_FATAL);
	}
	mine->f = f;
	return (archive_write_open2(a, mine, file_open, file_write,
	    NULL, file_free));
}
