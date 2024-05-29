void
archive_entry_set_gid(struct archive_entry *entry, la_int64_t g)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_gid = g;
}
