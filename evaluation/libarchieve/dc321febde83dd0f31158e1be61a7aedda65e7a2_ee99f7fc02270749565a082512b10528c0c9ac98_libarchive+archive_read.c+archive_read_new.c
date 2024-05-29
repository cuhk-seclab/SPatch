struct archive *
archive_read_new(void)
{
	struct archive_read *a;

	a = (struct archive_read *)calloc(1, sizeof(*a));
	if (a == NULL)
		return (NULL);
	a->archive.magic = ARCHIVE_READ_MAGIC;

	a->archive.state = ARCHIVE_STATE_NEW;
	a->entry = archive_entry_new2(&a->archive);
	a->archive.vtable = &archive_read_vtable;

	a->passphrases.last = &a->passphrases.first;

	return (&a->archive);
}
