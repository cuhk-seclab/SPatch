char *
archive_acl_to_text_l(struct archive_acl *acl, ssize_t *text_len, int flags,
    struct archive_string_conv *sc)
{
	int count;
	ssize_t length;
	size_t len;
	const char *name;
	const char *prefix;
	char separator;
	struct archive_acl_entry *ap;
	int id, r, want_type;
	char *p, *s;

	want_type = archive_acl_text_want_type(acl, flags);

	/* Both NFSv4 and POSIX.1 types found */
	if (want_type == 0)
		return (NULL);

	if (want_type == ARCHIVE_ENTRY_ACL_TYPE_POSIX1E)
		flags |= ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT;

	length = archive_acl_text_len(acl, want_type, flags, 0, NULL, sc);

	if (length == 0)
		return (NULL);

	if (flags & ARCHIVE_ENTRY_ACL_STYLE_SEPARATOR_COMMA)
		separator = ',';
	else
		separator = '\n';

	/* Now, allocate the string and actually populate it. */
	p = s = (char *)malloc(length * sizeof(char));
	if (p == NULL) {
		if (errno == ENOMEM)
			__archive_errx(1, "No memory");
		return (NULL);
	}
	count = 0;

	if ((want_type & ARCHIVE_ENTRY_ACL_TYPE_ACCESS) != 0) {
		append_entry(&p, NULL, ARCHIVE_ENTRY_ACL_TYPE_ACCESS,
		    ARCHIVE_ENTRY_ACL_USER_OBJ, flags, NULL,
		    acl->mode & 0700, -1);
		*p++ = separator;
		append_entry(&p, NULL, ARCHIVE_ENTRY_ACL_TYPE_ACCESS,
		    ARCHIVE_ENTRY_ACL_GROUP_OBJ, flags, NULL,
		    acl->mode & 0070, -1);
		*p++ = separator;
		append_entry(&p, NULL, ARCHIVE_ENTRY_ACL_TYPE_ACCESS,
		    ARCHIVE_ENTRY_ACL_OTHER, flags, NULL,
		    acl->mode & 0007, -1);
		count += 3;
	}

	for (ap = acl->acl_head; ap != NULL; ap = ap->next) {
		if ((ap->type & want_type) == 0)
			continue;
		/*
		 * Filemode-mapping ACL entries are stored exclusively in
		 * ap->mode so they should not be in the list
		 */
		if ((ap->type == ARCHIVE_ENTRY_ACL_TYPE_ACCESS)
		    && (ap->tag == ARCHIVE_ENTRY_ACL_USER_OBJ
		    || ap->tag == ARCHIVE_ENTRY_ACL_GROUP_OBJ
		    || ap->tag == ARCHIVE_ENTRY_ACL_OTHER))
			continue;
		if (ap->type == ARCHIVE_ENTRY_ACL_TYPE_DEFAULT &&
		    (flags & ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT) != 0)
			prefix = "default:";
		else
			prefix = NULL;
		r = archive_mstring_get_mbs_l(
		    &ap->name, &name, &len, sc);
		if (r != 0) {
			free(s);
			return (NULL);
		}
		if (count > 0)
			*p++ = separator;
		if (name == NULL ||
		    (flags & ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID)) {
			id = ap->id;
		} else {
			id = -1;
		}
		append_entry(&p, prefix, ap->type, ap->tag, flags, name,
		    ap->permset, id);
		count++;
	}

	/* Add terminating character */
	*p++ = '\0';

	len = strlen(s);

	if ((ssize_t)len > (length - 1))
		__archive_errx(1, "Buffer overrun");

	if (text_len != NULL)
		*text_len = len;

	return (s);
}
