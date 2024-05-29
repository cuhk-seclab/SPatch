int
archive_entry_sparse_next(struct archive_entry * entry,
	int64_t *offset, int64_t *length)
{
	if (entry->sparse_p) {
		*offset = entry->sparse_p->offset;
		*length = entry->sparse_p->length;

		entry->sparse_p = entry->sparse_p->next;

		return (ARCHIVE_OK);
	} else {
		*offset = 0;
		*length = 0;
		return (ARCHIVE_WARN);
	}
}
