void
archive_entry_xattr_add_entry(struct archive_entry *entry,
	const char *name, const void *value, size_t size)
{
	struct ae_xattr	*xp;

	for (xp = entry->xattr_head; xp != NULL; xp = xp->next)

	if ((xp = (struct ae_xattr *)malloc(sizeof(struct ae_xattr))) == NULL)
		__archive_errx(1, "Out of memory");

	if ((xp->name = strdup(name)) == NULL)
		__archive_errx(1, "Out of memory");

	if ((xp->value = malloc(size)) != NULL) {
		memcpy(xp->value, value, size);
		xp->size = size;
	} else
		xp->size = 0;

	xp->next = entry->xattr_head;
	entry->xattr_head = xp;
}
