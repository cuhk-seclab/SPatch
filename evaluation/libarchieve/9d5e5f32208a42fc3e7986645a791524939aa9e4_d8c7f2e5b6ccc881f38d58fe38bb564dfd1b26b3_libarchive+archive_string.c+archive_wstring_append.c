static struct archive_wstring *
archive_wstring_append(struct archive_wstring *as, const wchar_t *p, size_t s)
{
	if (archive_wstring_ensure(as, as->length + s + 1) == NULL)
		return (NULL);
	if (s)
		wmemmove(as->s + as->length, p, s);
	as->length += s;
	as->s[as->length] = 0;
	return (as);
}
