wchar_t *
archive_entry_acl_to_text_w(struct archive_entry *entry, la_ssize_t *len,
    int flags)
{
	return (archive_acl_to_text_w(&entry->acl, len, flags,
	    entry->archive));
}
