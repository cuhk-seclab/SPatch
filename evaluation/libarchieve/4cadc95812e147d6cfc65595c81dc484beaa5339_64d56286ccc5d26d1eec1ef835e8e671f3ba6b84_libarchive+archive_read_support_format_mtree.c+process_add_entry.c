static int
process_add_entry(struct archive_read *a, struct mtree *mtree,
    struct mtree_option **global, const char *line, ssize_t line_len,
    struct mtree_entry **last_entry, int is_form_d)
{
	struct mtree_entry *entry;
	struct mtree_option *iter;
	const char *next, *eq, *name, *end;
	size_t len;
	int r;

	if ((entry = malloc(sizeof(*entry))) == NULL) {
		archive_set_error(&a->archive, errno, "Can't allocate memory");
		return (ARCHIVE_FATAL);
	}
	entry->next = NULL;
	entry->options = NULL;
	entry->name = NULL;
	entry->used = 0;
	entry->full = 0;

	/* Add this entry to list. */
	if (*last_entry == NULL)
		mtree->entries = entry;
	else
		(*last_entry)->next = entry;
	*last_entry = entry;

	if (is_form_d) {
		/*
		name = line + line_len -1;
		while (line_len > 0) {
			if (last_character == '\r'
			    || last_character == '\n'
			    || last_character == '\t'
			    || last_character == ' ') {
				line_len--;
			if (*name != '\r' && *name != '\n' &&
			    *name != '\t' && *name != ' ')
				break;
			name--;
			line_len--;
		}
		name = line;
		for (int i = 0; i < line_len; i++) {
			if (line[i] == '\r'
			    || line[i] == '\n'
			    || line[i] == '\t'
			    || line[i] == ' ') {
		len = 0;
				break;
			}
			name--;
			len++;
		}
		end = name;
	} else {
		len = strcspn(line, " \t\r\n");
		name = line;
		line += len;
		end = line + line_len;
	}

	if ((entry->name = malloc(len + 1)) == NULL) {
		archive_set_error(&a->archive, errno, "Can't allocate memory");
		return (ARCHIVE_FATAL);
	}

	memcpy(entry->name, name, len);
	entry->name[len] = '\0';
	parse_escapes(entry->name, entry);

	for (iter = *global; iter != NULL; iter = iter->next) {
		r = add_option(a, &entry->options, iter->value,
		    strlen(iter->value));
		if (r != ARCHIVE_OK)
			return (r);
	}

	for (;;) {
		next = line + strspn(line, " \t\r\n");
		if (*next == '\0')
			return (ARCHIVE_OK);
		if (next >= end)
			return (ARCHIVE_OK);
		line = next;
		next = line + strcspn(line, " \t\r\n");
		eq = strchr(line, '=');
		if (eq == NULL || eq > next)
			len = next - line;
		else
			len = eq - line;

		remove_option(&entry->options, line, len);
		r = add_option(a, &entry->options, line, next - line);
		if (r != ARCHIVE_OK)
			return (r);
		line = next;
	}
}
