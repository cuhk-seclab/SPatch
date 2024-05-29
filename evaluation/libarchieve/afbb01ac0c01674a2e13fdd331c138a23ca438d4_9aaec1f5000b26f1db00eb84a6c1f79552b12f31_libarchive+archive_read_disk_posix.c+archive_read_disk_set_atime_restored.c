int
archive_read_disk_set_atime_restored(struct archive *_a)
{
	struct archive_read_disk *a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC,
	    ARCHIVE_STATE_ANY, "archive_read_disk_restore_atime");
#ifdef HAVE_UTIMES
	a->flags |= ARCHIVE_READDISK_RESTORE_ATIME;
	if (a->tree != NULL)
		a->tree->flags |= needsRestoreTimes;
	return (ARCHIVE_OK);
#else
	/* Display warning and unset flag */
	archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
	    "Cannot restore access time on this system");
	a->flags &= ~ARCHIVE_READDISK_RESTORE_ATIME;
	return (ARCHIVE_WARN);
#endif
}
