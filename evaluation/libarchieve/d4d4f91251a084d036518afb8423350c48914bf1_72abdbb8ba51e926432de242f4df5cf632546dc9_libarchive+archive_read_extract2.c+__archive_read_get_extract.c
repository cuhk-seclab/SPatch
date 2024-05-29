struct archive_read_extract *
__archive_read_get_extract(struct archive_read *a)
{
	if (a->extract == NULL) {
		a->extract = (struct archive_read_extract *)calloc(1, sizeof(*a->extract));
		if (a->extract == NULL) {
			archive_set_error(&a->archive, ENOMEM, "Can't extract");
			return (NULL);
		}
		a->cleanup_archive_extract = archive_read_extract_cleanup;
	}
	return (a->extract);
}
