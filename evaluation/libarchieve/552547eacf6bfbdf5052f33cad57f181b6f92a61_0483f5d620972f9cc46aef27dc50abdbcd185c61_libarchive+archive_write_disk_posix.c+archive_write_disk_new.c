struct archive *
archive_write_disk_new(void)
{
	struct archive_write_disk *a;

	a = (struct archive_write_disk *)calloc(1, sizeof(*a));
	if (a == NULL)
		return (NULL);
	a->archive.magic = ARCHIVE_WRITE_DISK_MAGIC;
	/* We're ready to write a header immediately. */
	a->archive.state = ARCHIVE_STATE_HEADER;
	a->archive.vtable = &archive_write_disk_vtable;
	a->start_time = time(NULL);
	/* Query and restore the umask. */
	umask(a->user_umask = umask(0));
#ifdef HAVE_GETEUID
	a->user_uid = geteuid();
#endif /* HAVE_GETEUID */
	if (archive_string_ensure(&a->path_safe, 512) == NULL) {
		free(a);
		return (NULL);
	}
	a->path_safe.s[0] = 0;

#ifdef HAVE_ZLIB_H
	a->decmpfs_compression_level = 5;
#endif
	return (&a->archive);
}
