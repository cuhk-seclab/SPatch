int
_archive_entry_pathname_l(struct archive_entry *entry,
    const char **p, size_t *len, struct archive_string_conv *sc)
{
	return (archive_mstring_get_mbs_l(entry->archive, &entry->ae_pathname, p, len, sc));
}
