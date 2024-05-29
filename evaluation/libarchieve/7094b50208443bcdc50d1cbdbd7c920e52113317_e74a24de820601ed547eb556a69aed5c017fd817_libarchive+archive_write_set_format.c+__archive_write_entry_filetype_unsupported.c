void
__archive_write_entry_filetype_unsupported(struct archive *a,
    struct archive_entry *entry, const char *format)
{
	const char *name = NULL;

	switch (archive_entry_filetype(entry)) {
	/*
	 * All formats should be able to archive regular files (AE_IFREG)
	 */
	case AE_IFDIR:
		name = "directories";
		break;
	case AE_IFLNK:
		name = "symbolic links";
		break;
	case AE_IFCHR:
		name = "character devices";
		break;
	case AE_IFBLK:
		name = "block devices";
		break;
	case AE_IFIFO:
		name = "named pipes";
		break;
	case AE_IFSOCK:
		name = "sockets";
		break;
	default:
		break;
	}

	if (name != NULL) {
		archive_set_error(a, ARCHIVE_ERRNO_FILE_FORMAT,
		    "%s: %s format cannot archive %s",
		    archive_entry_pathname(entry), format, name);
	} else {
		archive_set_error(a, ARCHIVE_ERRNO_FILE_FORMAT,
		    "%s: %s format cannot archive files with mode 0%lo",
		    archive_entry_pathname(entry), format,
		    (unsigned long)archive_entry_mode(entry));
	}
}
