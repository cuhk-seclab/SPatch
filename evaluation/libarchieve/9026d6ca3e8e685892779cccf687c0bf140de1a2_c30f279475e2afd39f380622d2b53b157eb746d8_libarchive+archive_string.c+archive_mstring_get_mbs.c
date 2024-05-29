int
archive_mstring_get_mbs(struct archive *a, struct archive_mstring *aes,
    const char **p)
{
	int r, ret = 0;

	/* If we already have an MBS form, return that immediately. */
	if (aes->aes_set & AES_SET_MBS) {
		*p = aes->aes_mbs.s;
		return (ret);
	}

	*p = NULL;
	/* If there's a WCS form, try converting with the native locale. */
	if (aes->aes_set & AES_SET_WCS) {
		archive_string_empty(&(aes->aes_mbs));
		r = archive_string_append_from_wcs(&(aes->aes_mbs),
		    aes->aes_wcs.s, aes->aes_wcs.length);
		*p = aes->aes_mbs.s;
		if (r == 0) {
			aes->aes_set |= AES_SET_MBS;
			return (ret);
		} else
			ret = -1;
	}

	/* If there's a UTF-8 form, try converting with the native locale. */
	if (aes->aes_set & AES_SET_UTF8) {
		archive_string_empty(&(aes->aes_mbs));
		sc = archive_string_conversion_from_charset(a, "UTF-8", 1);
		if (sc == NULL)
			return (-1);/* Couldn't allocate memory for sc. */
		r = archive_strncpy_l(&(aes->aes_mbs),
			aes->aes_utf8.s, aes->aes_utf8.length, sc);
		if (a == NULL)
			free_sconv_object(sc);
		*p = aes->aes_mbs.s;
		if (r == 0) {
			aes->aes_set |= AES_SET_MBS;
			ret = 0;/* success; overwrite previous error. */
		} else
			ret = -1;/* failure. */
	}
	return (ret);
}
