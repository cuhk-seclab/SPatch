int
archive_acl_text_l(struct archive_acl *acl, int flags,
    const char **acl_text, size_t *acl_text_len,
    struct archive_string_conv *sc)
{
	int count;
	size_t length;
	const char *name;
	const char *prefix;
	char separator;
	struct archive_acl_entry *ap;
	size_t len;
	int id, r;
	char *p;

	if (acl->acl_text != NULL) {
		free (acl->acl_text);
		acl->acl_text = NULL;
	}

	*acl_text = NULL;
	if (acl_text_len != NULL)
		*acl_text_len = 0;
	separator = ',';
	count = 0;
	length = 0;
	ap = acl->acl_head;
	while (ap != NULL) {
		if ((ap->type & flags) != 0) {
			count++;
			if ((flags & ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT) &&
			    (ap->type & ARCHIVE_ENTRY_ACL_TYPE_DEFAULT))
				length += 8; /* "default:" */
			length += 5; /* tag name */
			length += 1; /* colon */
			r = archive_mstring_get_mbs_l(
			    &ap->name, &name, &len, sc);
			if (r != 0)
				return (-1);
			if (len > 0 && name != NULL)
				length += len;
			else
				length += sizeof(uid_t) * 3 + 1;
			length ++; /* colon */
			length += 3; /* rwx */
			length += 1; /* colon */
			length += max(sizeof(uid_t), sizeof(gid_t)) * 3 + 1;
			length ++; /* newline */
		}
		ap = ap->next;
	}

	if (count > 0 && ((flags & ARCHIVE_ENTRY_ACL_TYPE_ACCESS) != 0)) {
		length += 10; /* "user::rwx\n" */
		length += 11; /* "group::rwx\n" */
		length += 11; /* "other::rwx\n" */
	}

	if (count == 0)
		return (0);

	/* Now, allocate the string and actually populate it. */
	p = acl->acl_text = (char *)malloc(length);
	if (p == NULL)
		return (-1);
	count = 0;
	if ((flags & ARCHIVE_ENTRY_ACL_TYPE_ACCESS) != 0) {
		append_entry(&p, NULL, ARCHIVE_ENTRY_ACL_USER_OBJ, NULL,
		    acl->mode & 0700, -1);
		*p++ = ',';
		append_entry(&p, NULL, ARCHIVE_ENTRY_ACL_GROUP_OBJ, NULL,
		    acl->mode & 0070, -1);
		*p++ = ',';
		append_entry(&p, NULL, ARCHIVE_ENTRY_ACL_OTHER, NULL,
		    acl->mode & 0007, -1);
		count += 3;

		for (ap = acl->acl_head; ap != NULL; ap = ap->next) {
			if ((ap->type & ARCHIVE_ENTRY_ACL_TYPE_ACCESS) == 0)
				continue;
			r = archive_mstring_get_mbs_l(
			    &ap->name, &name, &len, sc);
			if (r != 0)
				return (-1);
			*p++ = separator;
			if (flags & ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID)
				id = ap->id;
			else
				id = -1;
			}
			append_entry(&p, NULL, ap->tag, name,
			    ap->permset, id);
			count++;
		}
	}


	if ((flags & ARCHIVE_ENTRY_ACL_TYPE_DEFAULT) != 0) {
		if (flags & ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT)
			prefix = "default:";
		else
			prefix = NULL;
		count = 0;
		for (ap = acl->acl_head; ap != NULL; ap = ap->next) {
			if ((ap->type & ARCHIVE_ENTRY_ACL_TYPE_DEFAULT) == 0)
				continue;
			r = archive_mstring_get_mbs_l(
			    &ap->name, &name, &len, sc);
			if (r != 0)
				return (-1);
			if (count > 0)
				*p++ = separator;
			if (flags & ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID)
				id = ap->id;
			else
				id = -1;
			append_entry(&p, prefix, ap->tag,
			    name, ap->permset, id);
			count ++;
		}
	}

	*acl_text = acl->acl_text;
	if (acl_text_len != NULL)
		*acl_text_len = strlen(acl->acl_text);
	return (0);
}
