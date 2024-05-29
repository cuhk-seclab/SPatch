int
archive_match_include_gid(struct archive *_a, la_int64_t gid)
{
	struct archive_match *a;

	archive_check_magic(_a, ARCHIVE_MATCH_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_match_include_gid");
	a = (struct archive_match *)_a;
	return (add_owner_id(a, &(a->inclusion_gids), gid));
}
