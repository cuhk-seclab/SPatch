int
archive_entry_acl_types(struct archive_entry *entry)
{
	return (archive_acl_types(&entry->acl));
}
