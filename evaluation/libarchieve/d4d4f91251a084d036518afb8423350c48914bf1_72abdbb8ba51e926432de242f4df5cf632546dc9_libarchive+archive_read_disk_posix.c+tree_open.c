static struct tree *
tree_open(const char *path, int symlink_mode, int restore_time)
{
	struct tree *t;

	if ((t = calloc(1, sizeof(*t))) == NULL)
		return (NULL);
	archive_string_init(&t->path);
	archive_string_ensure(&t->path, 31);
	t->initial_symlink_mode = symlink_mode;
	return (tree_reopen(t, path, restore_time));
}
