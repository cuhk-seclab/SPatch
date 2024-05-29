int
archive_mstring_copy_wcs_len(struct archive_mstring *aes, const wchar_t *wcs,
    size_t len)
{
	if (wcs == NULL) {
		aes->aes_set = 0;
		return (0);
	}
	aes->aes_set = AES_SET_WCS; /* Only WCS form set. */
	archive_string_empty(&(aes->aes_mbs));
	archive_string_empty(&(aes->aes_utf8));
	archive_wstrncpy(&(aes->aes_wcs), wcs, len);
	return (0);
}
