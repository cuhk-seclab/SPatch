static struct archive_string *
archive_string_append(struct archive_string *as, const char *p, size_t s)
{
	if (s == 0)
		return (as);
	if (archive_string_ensure(as, as->length + s + 1) == NULL)
		return (NULL);
	memmove(as->s + as->length, p, s);
	as->length += s;
	as->s[as->length] = 0;
	return (as);
}
