static int
pax_attribute_xattr(struct archive_entry *entry,
	const char *name, const char *value)
{
	char *name_decoded;
	void *value_decoded;
	size_t value_len;

	if (strlen(name) < 18 || (memcmp(name, "LIBARCHIVE.xattr.", 17)) != 0)
		return 3;

	name += 17;

	/* URL-decode name */
	name_decoded = url_decode(name);
	if (name_decoded == NULL)
		return 2;

	/* Base-64 decode value */
	value_decoded = base64_decode(value, strlen(value), &value_len);
	if (value_decoded == NULL) {
		free(name_decoded);
		return 1;
	}

	archive_entry_xattr_add_entry(entry, name_decoded,
		value_decoded, value_len);

	free(name_decoded);
	free(value_decoded);
	return 0;
}
