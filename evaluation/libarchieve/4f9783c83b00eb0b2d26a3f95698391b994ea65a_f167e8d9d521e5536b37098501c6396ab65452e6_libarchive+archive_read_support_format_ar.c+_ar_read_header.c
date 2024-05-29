static int
_ar_read_header(struct archive_read *a, struct archive_entry *entry,
	struct ar *ar, const char *h, size_t *unconsumed)
{
	char filename[AR_name_size + 1];
	uint64_t number; /* Used to hold parsed numbers before validation. */
	size_t bsd_name_length, entry_size;
	char *p, *st;
	const void *b;
	int r;

	/* Verify the magic signature on the file header. */
	if (strncmp(h + AR_fmag_offset, "`\n", 2) != 0) {
		archive_set_error(&a->archive, EINVAL,
		    "Incorrect file header signature");
		return (ARCHIVE_FATAL);
	}

	/* Copy filename into work buffer. */
	strncpy(filename, h + AR_name_offset, AR_name_size);
	filename[AR_name_size] = '\0';

	/*
	 * Guess the format variant based on the filename.
	 */
	if (a->archive.archive_format == ARCHIVE_FORMAT_AR) {
		/* We don't already know the variant, so let's guess. */
		/*
		 * Biggest clue is presence of '/': GNU starts special
		 * filenames with '/', appends '/' as terminator to
		 * non-special names, so anything with '/' should be
		 * GNU except for BSD long filenames.
		 */
		if (strncmp(filename, "#1/", 3) == 0)
			a->archive.archive_format = ARCHIVE_FORMAT_AR_BSD;
		else if (strchr(filename, '/') != NULL)
			a->archive.archive_format = ARCHIVE_FORMAT_AR_GNU;
		else if (strncmp(filename, "__.SYMDEF", 9) == 0)
			a->archive.archive_format = ARCHIVE_FORMAT_AR_BSD;
		/*
		 * XXX Do GNU/SVR4 'ar' programs ever omit trailing '/'
		 * if name exactly fills 16-byte field?  If so, we
		 * can't assume entries without '/' are BSD. XXX
		 */
	}

	/* Update format name from the code. */
	if (a->archive.archive_format == ARCHIVE_FORMAT_AR_GNU)
		a->archive.archive_format_name = "ar (GNU/SVR4)";
	else if (a->archive.archive_format == ARCHIVE_FORMAT_AR_BSD)
		a->archive.archive_format_name = "ar (BSD)";
	else
		a->archive.archive_format_name = "ar";

	/*
	 * Remove trailing spaces from the filename.  GNU and BSD
	 * variants both pad filename area out with spaces.
	 * This will only be wrong if GNU/SVR4 'ar' implementations
	 * omit trailing '/' for 16-char filenames and we have
	 * a 16-char filename that ends in ' '.
	 */
	p = filename + AR_name_size - 1;
	while (p >= filename && *p == ' ') {
		*p = '\0';
		p--;
	}

	/*
	 * Remove trailing slash unless first character is '/'.
	 * (BSD entries never end in '/', so this will only trim
	 * GNU-format entries.  GNU special entries start with '/'
	 * and are not terminated in '/', so we don't trim anything
	 * that starts with '/'.)
	 */
	if (filename[0] != '/' && p > filename && *p == '/') {
		*p = '\0';
	}

	if (p < filename) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
		    "Found entry with empty filename");
		return (ARCHIVE_FATAL);
	}

	/*
	 * '//' is the GNU filename table.
	 * Later entries can refer to names in this table.
	 */
	if (strcmp(filename, "//") == 0) {
		/* This must come before any call to _read_ahead. */
		ar_parse_common_header(ar, entry, h);
		archive_entry_copy_pathname(entry, filename);
		archive_entry_set_filetype(entry, AE_IFREG);
		/* Get the size of the filename table. */
		number = ar_atol10(h + AR_size_offset, AR_size_size);
		if (number > SIZE_MAX || number > 1024 * 1024 * 1024) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
			    "Filename table too large");
			return (ARCHIVE_FATAL);
		}
		entry_size = (size_t)number;
		if (entry_size == 0) {
			archive_set_error(&a->archive, EINVAL,
			    "Invalid string table");
			return (ARCHIVE_FATAL);
		}
		if (ar->strtab != NULL) {
			archive_set_error(&a->archive, EINVAL,
			    "More than one string tables exist");
			return (ARCHIVE_FATAL);
		}

		/* Read the filename table into memory. */
		st = malloc(entry_size);
		if (st == NULL) {
			archive_set_error(&a->archive, ENOMEM,
			    "Can't allocate filename table buffer");
			return (ARCHIVE_FATAL);
		}
		ar->strtab = st;
		ar->strtab_size = entry_size;

		if (*unconsumed) {
			__archive_read_consume(a, *unconsumed);
			*unconsumed = 0;
		}

		if ((b = __archive_read_ahead(a, entry_size, NULL)) == NULL)
			return (ARCHIVE_FATAL);
		memcpy(st, b, entry_size);
		__archive_read_consume(a, entry_size);
		/* All contents are consumed. */
		ar->entry_bytes_remaining = 0;
		archive_entry_set_size(entry, ar->entry_bytes_remaining);

		/* Parse the filename table. */
		return (ar_parse_gnu_filename_table(a));
	}

	/*
	 * GNU variant handles long filenames by storing /<number>
	 * to indicate a name stored in the filename table.
	 * XXX TODO: Verify that it's all digits... Don't be fooled
	 * by "/9xyz" XXX
	 */
	if (filename[0] == '/' && filename[1] >= '0' && filename[1] <= '9') {
		number = ar_atol10(h + AR_name_offset + 1, AR_name_size - 1);
		/*
		 * If we can't look up the real name, warn and return
		 * the entry with the wrong name.
		 */
		if (ar->strtab == NULL || number >= ar->strtab_size) {
			archive_set_error(&a->archive, EINVAL,
			    "Can't find long filename for GNU/SVR4 archive entry");
			archive_entry_copy_pathname(entry, filename);
			/* Parse the time, owner, mode, size fields. */
			ar_parse_common_header(ar, entry, h);
			return (ARCHIVE_FATAL);
		}

		archive_entry_copy_pathname(entry, &ar->strtab[(size_t)number]);
		/* Parse the time, owner, mode, size fields. */
		return (ar_parse_common_header(ar, entry, h));
	}

	/*
	 * BSD handles long filenames by storing "#1/" followed by the
	 * length of filename as a decimal number, then prepends the
	 * the filename to the file contents.
	 */
	if (strncmp(filename, "#1/", 3) == 0) {
		/* Parse the time, owner, mode, size fields. */
		/* This must occur before _read_ahead is called again. */
		ar_parse_common_header(ar, entry, h);

		/* Parse the size of the name, adjust the file size. */
		number = ar_atol10(h + AR_name_offset + 3, AR_name_size - 3);
		/* Sanity check the filename length:
		 *   = Must be <= SIZE_MAX - 1
		 *   = Must be <= 1MB
		 *   = Cannot be bigger than the entire entry
		 */
		if (number > SIZE_MAX - 1
		    || number > 1024 * 1024
		    || (int64_t)number > ar->entry_bytes_remaining) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
			    "Bad input file size");
			return (ARCHIVE_FATAL);
		}
		bsd_name_length = (size_t)number;
		ar->entry_bytes_remaining -= bsd_name_length;
		/* Adjust file size reported to client. */
		archive_entry_set_size(entry, ar->entry_bytes_remaining);

		if (*unconsumed) {
			__archive_read_consume(a, *unconsumed);
			*unconsumed = 0;
		}

		/* Read the long name into memory. */
		if ((b = __archive_read_ahead(a, bsd_name_length, NULL)) == NULL) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
			    "Truncated input file");
			return (ARCHIVE_FATAL);
		}
		/* Store it in the entry. */
		p = (char *)malloc(bsd_name_length + 1);
		if (p == NULL) {
			archive_set_error(&a->archive, ENOMEM,
			    "Can't allocate fname buffer");
			return (ARCHIVE_FATAL);
		}
		strncpy(p, b, bsd_name_length);
		p[bsd_name_length] = '\0';

		__archive_read_consume(a, bsd_name_length);

		archive_entry_copy_pathname(entry, p);
		free(p);
		return (ARCHIVE_OK);
	}

	/*
	 * "/" is the SVR4/GNU archive symbol table.
	 */
	if (strcmp(filename, "/") == 0) {
		archive_entry_copy_pathname(entry, "/");
		/* Parse the time, owner, mode, size fields. */
		r = ar_parse_common_header(ar, entry, h);
		/* Force the file type to a regular file. */
		archive_entry_set_filetype(entry, AE_IFREG);
		return (r);
	}

	/*
	 * "__.SYMDEF" is a BSD archive symbol table.
	 */
	if (strcmp(filename, "__.SYMDEF") == 0) {
		archive_entry_copy_pathname(entry, filename);
		/* Parse the time, owner, mode, size fields. */
		return (ar_parse_common_header(ar, entry, h));
	}

	/*
	 * Otherwise, this is a standard entry.  The filename
	 * has already been trimmed as much as possible, based
	 * on our current knowledge of the format.
	 */
	archive_entry_copy_pathname(entry, filename);
	return (ar_parse_common_header(ar, entry, h));
}
