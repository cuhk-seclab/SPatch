static int
read_mtree(struct archive_read *a, struct mtree *mtree)
{
	ssize_t len;
	uintmax_t counter;
	char *p, *s;
	struct mtree_option *global;
	struct mtree_entry *last_entry;
	int r, is_form_d;

	mtree->archive_format = ARCHIVE_FORMAT_MTREE;
	mtree->archive_format_name = "mtree";

	global = NULL;
	last_entry = NULL;

	(void)detect_form(a, &is_form_d);

	for (counter = 1; ; ++counter) {
		len = readline(a, mtree, &p, 65536);
		if (len == 0) {
			mtree->this_entry = mtree->entries;
			free_options(global);
			return (ARCHIVE_OK);
		}
		if (len < 0) {
			free_options(global);
			return ((int)len);
		}
		/* Leading whitespace is never significant, ignore it. */
		while (*p == ' ' || *p == '\t') {
			++p;
			--len;
		}
		/* Skip content lines and blank lines. */
		if (*p == '#')
			continue;
		if (*p == '\r' || *p == '\n' || *p == '\0')
			continue;
		for (s = p;s < p + len - 1; s++) {
			if (!isprint(*s)) {
				r = ARCHIVE_FATAL;
				break;
			}
		}
		if (r != ARCHIVE_OK)
		if (*p != '/') {
			r = process_add_entry(a, mtree, &global, p, len,
			    &last_entry, is_form_d);
		} else if (len > 4 && strncmp(p, "/set", 4) == 0) {
			if (p[4] != ' ' && p[4] != '\t')
				break;
			r = process_global_set(a, &global, p);
		} else if (len > 6 && strncmp(p, "/unset", 6) == 0) {
			if (p[6] != ' ' && p[6] != '\t')
				break;
			r = process_global_unset(a, &global, p);
		} else
			break;

		if (r != ARCHIVE_OK) {
			free_options(global);
			return r;
		}
	}

	archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
	    "Can't parse line %ju", counter);
	free_options(global);
	return (ARCHIVE_FATAL);
}
