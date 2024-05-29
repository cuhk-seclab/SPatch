struct archive *
archive_read_disk_new(void)
{
	struct archive_read_disk *a;

	a = (struct archive_read_disk *)calloc(1, sizeof(*a));
	if (a == NULL)
		return (NULL);
	a->archive.magic = ARCHIVE_READ_DISK_MAGIC;
	a->archive.state = ARCHIVE_STATE_NEW;
	a->archive.vtable = &archive_read_disk_vtable;
	a->entry = archive_entry_new2(&a->archive);
	a->lookup_uname = trivial_lookup_uname;
	a->lookup_gname = trivial_lookup_gname;
	a->flags = ARCHIVE_READDISK_MAC_COPYFILE;
	a->open_on_current_dir = open_on_current_dir;
	a->tree_current_dir_fd = tree_current_dir_fd;
	a->tree_enter_working_dir = tree_enter_working_dir;
	return (&a->archive);
}
