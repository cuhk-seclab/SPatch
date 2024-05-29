static struct fixup_entry *
new_fixup(struct archive_write_disk *a, const char *pathname)
{
	struct fixup_entry *fe;

	fe = (struct fixup_entry *)calloc(1, sizeof(struct fixup_entry));
	if (fe == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate memory for a fixup");
		return (NULL);
	}
	fe->next = a->fixup_list;
	a->fixup_list = fe;
	fe->fixup = 0;
	fe->name = strdup(pathname);
	fe->fd = -1;
	return (fe);
}
