int
_archive_set_options(struct archive *a, const char *options,
    int magic, const char *fn, option_handler use_option)
{
	int allok = 1, anyok = 0, ignore_mod_err = 0, r;
	char *data;
	const char *s, *mod, *opt, *val;

	archive_check_magic(a, magic, ARCHIVE_STATE_NEW, fn);

	if (options == NULL || options[0] == '\0')
		return ARCHIVE_OK;

	if ((data = strdup(options)) == NULL) {
		archive_set_error(a,
		    ENOMEM, "Out of memory adding file to list");
		return (ARCHIVE_FATAL);
	}
	s = (const char *)data;

	do {
		mod = opt = val = NULL;

		parse_option(&s, &mod, &opt, &val);
		if (mod == NULL && opt != NULL &&
		    strcmp("__ignore_wrong_module_name__", opt) == 0) {
			/* Ignore module name error */
			if (val != NULL) {
				ignore_mod_err = 1;
				anyok = 1;
			}
			continue;
		}

		r = use_option(a, mod, opt, val);
		if (r == ARCHIVE_FATAL) {
			free(data);
			return (ARCHIVE_FATAL);
		}
		if (r == ARCHIVE_FAILED && mod != NULL) {
			free(data);
			return (ARCHIVE_FAILED);
		}
		if (r == ARCHIVE_WARN - 1) {
			if (ignore_mod_err)
				continue;
			/* The module name is wrong. */
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unknown module name: `%s'", mod);
			free(data);
			return (ARCHIVE_FAILED);
		}
		if (r == ARCHIVE_WARN) {
			/* The option name is wrong. No-one used this. */
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Undefined option: `%s%s%s'",
			    mod?mod:"", mod?":":"", opt);
			free(data);
			return (ARCHIVE_FAILED);
		}
		if (r == ARCHIVE_OK)
			anyok = 1;
		else
			allok = 0;
	} while (s != NULL);

	free(data);
	return allok ? ARCHIVE_OK : anyok ? ARCHIVE_WARN : ARCHIVE_FAILED;
}
