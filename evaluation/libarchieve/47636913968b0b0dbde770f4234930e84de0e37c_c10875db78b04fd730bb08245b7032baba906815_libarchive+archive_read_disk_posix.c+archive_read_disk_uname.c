const char *
archive_read_disk_uname(struct archive *_a, la_int64_t uid)
{
	struct archive_read_disk *a = (struct archive_read_disk *)_a;
	if (ARCHIVE_OK != __archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC,
		ARCHIVE_STATE_ANY, "archive_read_disk_uname"))
		return (NULL);
	if (a->lookup_uname == NULL)
		return (NULL);
	return ((*a->lookup_uname)(a->lookup_uname_data, uid));
}
