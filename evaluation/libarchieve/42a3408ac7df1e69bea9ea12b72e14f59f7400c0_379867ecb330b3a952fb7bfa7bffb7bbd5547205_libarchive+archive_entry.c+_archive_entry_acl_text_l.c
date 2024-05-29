int
_archive_entry_acl_text_l(struct archive_entry *entry, int flags,
    const char **acl_text, size_t *len, struct archive_string_conv *sc)
{
	if (entry->acl.acl_text != NULL) {
		free(entry->acl.acl_text);
		entry->acl.acl_text = NULL;
        }

	if (archive_entry_acl_text_compat(&flags) == 0)
		entry->acl.acl_text = archive_acl_to_text_l(&entry->acl,
		    (ssize_t *)len, flags, sc);

	*acl_text = entry->acl.acl_text;

	return (0);
}
