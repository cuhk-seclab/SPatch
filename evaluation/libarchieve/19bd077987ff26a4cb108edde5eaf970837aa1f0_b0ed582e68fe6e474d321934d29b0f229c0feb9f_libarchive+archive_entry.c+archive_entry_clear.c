struct archive_entry *
archive_entry_clear(struct archive_entry *entry)
{
	if (entry == NULL)
		return (NULL);
	archive_mstring_clean(&entry->ae_fflags_text);
	archive_mstring_clean(&entry->ae_gname);
	archive_mstring_clean(&entry->ae_hardlink);
	archive_mstring_clean(&entry->ae_pathname);
	archive_mstring_clean(&entry->ae_sourcepath);
	archive_mstring_clean(&entry->ae_symlink);
	archive_mstring_clean(&entry->ae_uname);
	archive_entry_copy_mac_metadata(entry, NULL, 0);
	archive_acl_clear(&entry->acl);
	archive_entry_xattr_clear(entry);
	archive_entry_sparse_clear(entry);
	free(entry->stat);
	entry->ae_symlink_type = AE_SYMLINK_TYPE_UNDEFINED;
	memset(entry, 0, sizeof(*entry));
	return entry;
}
