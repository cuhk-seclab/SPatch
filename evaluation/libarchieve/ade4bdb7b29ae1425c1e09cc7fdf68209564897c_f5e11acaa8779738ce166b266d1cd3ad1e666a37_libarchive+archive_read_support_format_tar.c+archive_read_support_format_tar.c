int
archive_read_support_format_tar(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct tar *tar;
	int r;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_format_tar");

	tar = (struct tar *)calloc(1, sizeof(*tar));
	if (tar == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate tar data");
		return (ARCHIVE_FATAL);
	}
#ifdef HAVE_COPYFILE_H
	/* Set this by default on Mac OS. */
	tar->process_mac_extensions = 1;
#endif

	r = __archive_read_register_format(a, tar, "tar",
	    archive_read_format_tar_bid,
	    archive_read_format_tar_options,
	    archive_read_format_tar_read_header,
	    archive_read_format_tar_read_data,
	    archive_read_format_tar_skip,
	    NULL,
	    archive_read_format_tar_cleanup,
	    NULL,
	    NULL);

	if (r != ARCHIVE_OK)
		free(tar);
	return (ARCHIVE_OK);
}
