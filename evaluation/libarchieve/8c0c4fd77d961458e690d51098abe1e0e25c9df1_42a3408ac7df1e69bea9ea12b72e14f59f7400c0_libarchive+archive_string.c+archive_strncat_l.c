int
archive_strncat_l(struct archive_string *as, const void *_p, size_t n,
    struct archive_string_conv *sc)
{
	const void *s;
	size_t length = 0;
	int i, r = 0, r2;

	if (_p != NULL && n > 0) {
		if (sc != NULL && (sc->flag & SCONV_FROM_UTF16))
			length = utf16nbytes(_p, n);
		else
			length = mbsnbytes(_p, n);
	}

	/* We must allocate memory even if there is no data for conversion
	 * or copy. This simulates archive_string_append behavior. */
	if (length == 0) {
		int tn = 1;
		if (sc != NULL && (sc->flag & SCONV_TO_UTF16))
			tn = 2;
		if (archive_string_ensure(as, as->length + tn) == NULL)
			return (-1);
		as->s[as->length] = 0;
		if (tn == 2)
			as->s[as->length+1] = 0;
		return (0);
	}

	/*
	 * If sc is NULL, we just make a copy.
	 */
	if (sc == NULL) {
		if (archive_string_append(as, _p, length) == NULL)
			return (-1);/* No memory */
		return (0);
	}

	s = _p;
	i = 0;
	if (sc->nconverter > 1) {
		sc->utftmp.length = 0;
		r2 = sc->converter[0](&(sc->utftmp), s, length, sc);
		if (r2 != 0 && errno == ENOMEM)
			return (r2);
		if (r > r2)
			r = r2;
		s = sc->utftmp.s;
		length = sc->utftmp.length;
		++i;
	}
	r2 = sc->converter[i](as, s, length, sc);
	if (r > r2)
		r = r2;
	return (r);
}
