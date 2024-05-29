int
archive_write_disk_set_standard_lookup(struct archive *a)
{
	struct bucket *ucache = calloc(cache_size, sizeof(struct bucket));
	struct bucket *gcache = calloc(cache_size, sizeof(struct bucket));
	if (ucache == NULL || gcache == NULL) {
		free(ucache);
		free(gcache);
		return (ARCHIVE_FATAL);
	}
	archive_write_disk_set_group_lookup(a, gcache, lookup_gid, cleanup);
	archive_write_disk_set_user_lookup(a, ucache, lookup_uid, cleanup);
	return (ARCHIVE_OK);
}
