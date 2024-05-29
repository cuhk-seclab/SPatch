struct archive *
archive_match_new(void)
{
	struct archive_match *a;

	a = (struct archive_match *)calloc(1, sizeof(*a));
	if (a == NULL)
		return (NULL);
	a->archive.magic = ARCHIVE_MATCH_MAGIC;
	a->archive.state = ARCHIVE_STATE_NEW;
	a->recursive_include = 1;
	match_list_init(&(a->inclusions));
	match_list_init(&(a->exclusions));
	__archive_rb_tree_init(&(a->exclusion_tree), &rb_ops_mbs);
	entry_list_init(&(a->exclusion_entry_list));
	match_list_init(&(a->inclusion_unames));
	match_list_init(&(a->inclusion_gnames));
	time(&a->now);
	return (&(a->archive));
}
