static int
process_add_entry(struct archive_read *a, struct mtree *mtree,
    struct mtree_option **global, const char *line, ssize_t line_len,
    struct mtree_entry **last_entry, int is_form_d)
{
	struct mtree_entry *entry;
	struct mtree_option *iter;
	const char *next, *eq, *name, *end;
	size_t name_len, len;
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
		/* Filename is last item on line. */
		/* Adjust line_len to trim trailing whitespace */
		while (line_len > 0) {
			char last_character = line[line_len - 1];
			if (last_character == '\r'
			    || last_character == '\n'
			    || last_character == '\t'
			    || last_character == ' ') {
				line_len--;
			} else {
				break;
			}
		}
		/* Name starts after the last whitespace separator */
		name = line;
		for (int i = 0; i < line_len; i++) {
			if (line[i] == '\r'
			    || line[i] == '\n'
			    || line[i] == '\t'
			    || line[i] == ' ') {
				name = line + i + 1;
			}
		}
		name_len = line + line_len - name;
		end = name;
	} else {
		/* Filename is first item on line */
		name_len = strcspn(line, " \t\r\n");
		name = line;
		line += name_len;
		end = line + line_len;
	}
	/* name/name_len is the name within the line. */
	/* line..end brackets the entire line except the name */

	if ((entry->name = malloc(name_len + 1)) == NULL) {
		archive_set_error(&a->archive, errno, "Can't allocate memory");
		return (ARCHIVE_FATAL);
	}

	memcpy(entry->name, name, name_len);
	entry->name[name_len] = '\0';
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
