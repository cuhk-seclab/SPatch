int
archive_write_set_format_v7tar(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct v7tar *v7tar;

	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_set_format_v7tar");

	/* If someone else was already registered, unregister them. */
	if (a->format_free != NULL)
		(a->format_free)(a);

	/* Basic internal sanity test. */
	if (sizeof(template_header) != 512) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
		    "Internal: template_header wrong size: %zu should be 512",
		    sizeof(template_header));
		return (ARCHIVE_FATAL);
	}

	v7tar = (struct v7tar *)calloc(1, sizeof(*v7tar));
	if (v7tar == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate v7tar data");
		return (ARCHIVE_FATAL);
	}
	a->format_data = v7tar;
	a->format_name = "tar (non-POSIX)";
	a->format_options = archive_write_v7tar_options;
	a->format_write_header = archive_write_v7tar_header;
	a->format_write_data = archive_write_v7tar_data;
	a->format_close = archive_write_v7tar_close;
	a->format_free = archive_write_v7tar_free;
	a->format_finish_entry = archive_write_v7tar_finish_entry;
	a->archive.archive_format = ARCHIVE_FORMAT_TAR;
	a->archive.archive_format_name = "tar (non-POSIX)";
	return (ARCHIVE_OK);
}
