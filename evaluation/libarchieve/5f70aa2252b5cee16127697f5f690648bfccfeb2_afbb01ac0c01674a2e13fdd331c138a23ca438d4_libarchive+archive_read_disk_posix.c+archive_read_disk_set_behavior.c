int
archive_read_disk_set_behavior(struct archive *_a, int flags)
{
	struct archive_read_disk *a = (struct archive_read_disk *)_a;
	int r = ARCHIVE_OK;

	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC,
	    ARCHIVE_STATE_ANY, "archive_read_disk_honor_nodump");

	if (flags & ARCHIVE_READDISK_RESTORE_ATIME)
		r = archive_read_disk_set_atime_restored(_a);
	else {
		a->restore_time = 0;
		if (a->tree != NULL)
			a->tree->flags &= ~needsRestoreTimes;
	}
	if (flags & ARCHIVE_READDISK_HONOR_NODUMP)
		a->honor_nodump = 1;
	else
		a->honor_nodump = 0;
	if (flags & ARCHIVE_READDISK_MAC_COPYFILE)
		a->enable_copyfile = 1;
	else
		a->enable_copyfile = 0;
	if (flags & ARCHIVE_READDISK_NO_TRAVERSE_MOUNTS)
		a->traverse_mount_points = 0;
	else
		a->traverse_mount_points = 1;
	if (flags & ARCHIVE_READDISK_NO_XATTR)
		a->suppress_xattr = 1;
	else
		a->suppress_xattr = 0;
	if (flags & ARCHIVE_READDISK_NO_ACL)
		a->suppress_acl = 1;
	else
		a->suppress_acl = 0;
	return (r);
}
