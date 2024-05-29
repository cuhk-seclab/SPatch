const char *
archive_entry_pathname(struct archive_entry *entry)
{
	const char *p;
	if (archive_mstring_get_mbs(
	    entry->archive, &entry->ae_pathname, &p) == 0)
		return (p);
#if HAVE_EILSEQ  /*{*/
    if (errno == EILSEQ) {
	    if (archive_mstring_get_utf8(
	        entry->archive, &entry->ae_pathname, &p) == 0)
		    return (p);
    }
#endif  /*}*/
	if (errno == ENOMEM)
		__archive_errx(1, "No memory");
	return (NULL);
}
