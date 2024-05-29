static void
add_converter(struct archive_string_conv *sc, int (*converter)
    (struct archive_string *, const void *, size_t,
     struct archive_string_conv *))
{
	if (sc == NULL || sc->nconverter >= 2)
		__archive_errx(1, "Programming error");
	sc->converter[sc->nconverter++] = converter;
}
