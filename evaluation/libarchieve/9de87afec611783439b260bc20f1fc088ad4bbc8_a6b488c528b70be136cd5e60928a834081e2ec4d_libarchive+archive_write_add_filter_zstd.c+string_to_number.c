static int string_to_number(const char *string, intmax_t *numberp)
{
	char *end;

	if (string == NULL || *string == '\0')
		return (ARCHIVE_WARN);
	*numberp = strtoimax(string, &end, 10);
	if (end == string || *end != '\0' || errno == EOVERFLOW) {
		*numberp = 0;
		return (ARCHIVE_WARN);
	}
	return (ARCHIVE_OK);
}
