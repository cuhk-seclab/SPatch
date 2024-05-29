static void
append_uint(struct archive_string *as, uintmax_t d, unsigned base)
{
	static const char digits[] = "0123456789abcdef";
	if (d >= base)
		append_uint(as, d/base, base);
	archive_strappend_char(as, digits[d % base]);
}
