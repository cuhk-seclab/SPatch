const char *
archive_version_details(void)
{
	static struct archive_string str;
	static int init = 0;
	const char *zlib = archive_zlib_version();
	const char *liblzma = archive_liblzma_version();
	const char *bzlib = archive_bzlib_version();
	const char *liblz4 = archive_liblz4_version();
	const char *libzstd = archive_libzstd_version();

	if (!init) {
		archive_string_init(&str);

		archive_strcat(&str, ARCHIVE_VERSION_STRING);
		if (zlib != NULL) {
			archive_strcat(&str, " zlib/");
			archive_strcat(&str, zlib);
		}
		if (liblzma) {
			archive_strcat(&str, " liblzma/");
			archive_strcat(&str, liblzma);
		}
		if (bzlib) {
			const char *p = bzlib;
			const char *sep = strchr(p, ',');
			if (sep == NULL)
				sep = p + strlen(p);
			archive_strcat(&str, " bz2lib/");
			archive_strncat(&str, p, sep - p);
		}
		if (liblz4) {
			archive_strcat(&str, " liblz4/");
			archive_strcat(&str, liblz4);
		}
		if (libzstd) {
			archive_strcat(&str, " libzstd/");
			archive_strcat(&str, libzstd);
		}
	}
	return str.s;
}
