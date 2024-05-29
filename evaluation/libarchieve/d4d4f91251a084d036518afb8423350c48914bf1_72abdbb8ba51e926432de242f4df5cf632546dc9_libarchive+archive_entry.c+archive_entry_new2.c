struct archive_entry *
archive_entry_new2(struct archive *a)
{
	struct archive_entry *entry;

	entry = (struct archive_entry *)calloc(1, sizeof(*entry));
	if (entry == NULL)
		return (NULL);
	entry->archive = a;
	return (entry);
}
