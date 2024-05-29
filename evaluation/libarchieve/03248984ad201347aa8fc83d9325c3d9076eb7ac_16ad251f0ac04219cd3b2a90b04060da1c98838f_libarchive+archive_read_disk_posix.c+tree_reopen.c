static struct tree *
tree_reopen(struct tree *t, const char *path, int restore_time)
{
#if defined(O_PATH)
	/* Linux */
	const int o_flag = O_PATH;
#elif defined(O_SEARCH)
	/* SunOS */
	const int o_flag = O_SEARCH;
#elif defined(__FreeBSD__) && defined(O_EXEC)
	/* FreeBSD */
	const int o_flag = O_EXEC;
#endif

	t->flags = (restore_time != 0)?needsRestoreTimes:0;
	t->flags |= onInitialDir;
	t->visit_type = 0;
	t->tree_errno = 0;
	t->dirname_length = 0;
	t->depth = 0;
	t->descend = 0;
	t->current = NULL;
	t->d = INVALID_DIR_HANDLE;
	t->symlink_mode = t->initial_symlink_mode;
	archive_string_empty(&t->path);
	t->entry_fd = -1;
	t->entry_eof = 0;
	t->entry_remaining_bytes = 0;
	t->initial_filesystem_id = -1;

	/* First item is set up a lot like a symlink traversal. */
	tree_push(t, path, 0, 0, 0, NULL);
	t->stack->flags = needsFirstVisit;
	t->maxOpenCount = t->openCount = 1;
	t->initial_dir_fd = open(".", O_RDONLY | O_CLOEXEC);
#if defined(O_PATH) || defined(O_SEARCH) || \
 (defined(__FreeBSD__) && defined(O_EXEC))
	/*
	 * Most likely reason to fail opening "." is that it's not readable,
	 * so try again for execute. The consequences of not opening this are
	 * unhelpful and unnecessary errors later.
	 */
	if (t->initial_dir_fd < 0)
		t->initial_dir_fd = open(".", o_flag | O_CLOEXEC);
#endif
	__archive_ensure_cloexec_flag(t->initial_dir_fd);
	t->working_dir_fd = tree_dup(t->initial_dir_fd);
	return (t);
}
