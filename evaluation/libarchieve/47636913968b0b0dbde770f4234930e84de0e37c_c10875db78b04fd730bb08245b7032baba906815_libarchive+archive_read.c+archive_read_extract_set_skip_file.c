void
archive_read_extract_set_skip_file(struct archive *_a, la_int64_t d,
    la_int64_t i)
{
	struct archive_read *a = (struct archive_read *)_a;

	if (ARCHIVE_OK != __archive_check_magic(_a, ARCHIVE_READ_MAGIC,
		ARCHIVE_STATE_ANY, "archive_read_extract_set_skip_file"))
		return;
	a->skip_file_set = 1;
	a->skip_file_dev = d;
	a->skip_file_ino = i;
}
