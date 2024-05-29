int
__archive_write_format_header_ustar(struct archive_write *a, char h[512],
    struct archive_entry *entry, int tartype, int strict,
    struct archive_string_conv *sconv)
{
	unsigned int checksum;
	int i, r, ret;
	size_t copy_length;
	const char *p, *pp;
	int mytartype;

	ret = 0;
	mytartype = -1;
	/*
	 * The "template header" already includes the "ustar"
	 * signature, various end-of-field markers and other required
	 * elements.
	 */
	memcpy(h, &template_header, 512);

	/*
	 * Because the block is already null-filled, and strings
	 * are allowed to exactly fill their destination (without null),
	 * I use memcpy(dest, src, strlen()) here a lot to copy strings.
	 */
	r = archive_entry_pathname_l(entry, &pp, &copy_length, sconv);
	if (r != 0) {
		if (errno == ENOMEM) {
			archive_set_error(&a->archive, ENOMEM,
			    "Can't allocate memory for Pathname");
			return (ARCHIVE_FATAL);
		}
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Can't translate pathname '%s' to %s",
		    pp, archive_string_conversion_charset_name(sconv));
		ret = ARCHIVE_WARN;
	}
	if (copy_length <= USTAR_name_size)
		memcpy(h + USTAR_name_offset, pp, copy_length);
	else {
		/* Store in two pieces, splitting at a '/'. */
		p = strchr(pp + copy_length - USTAR_name_size - 1, '/');
		/*
		 * Look for the next '/' if we chose the first character
		 * as the separator.  (ustar format doesn't permit
		 * an empty prefix.)
		 */
		if (p == pp)
			p = strchr(p + 1, '/');
		/* Fail if the name won't fit. */
		if (!p) {
			/* No separator. */
			archive_set_error(&a->archive, ENAMETOOLONG,
			    "Pathname too long");
			ret = ARCHIVE_FAILED;
		} else if (p[1] == '\0') {
			/*
			 * The only feasible separator is a final '/';
			 * this would result in a non-empty prefix and
			 * an empty name, which POSIX doesn't
			 * explicitly forbid, but it just feels wrong.
			 */
			archive_set_error(&a->archive, ENAMETOOLONG,
			    "Pathname too long");
			ret = ARCHIVE_FAILED;
		} else if (p  > pp + USTAR_prefix_size) {
			/* Prefix is too long. */
			archive_set_error(&a->archive, ENAMETOOLONG,
			    "Pathname too long");
			ret = ARCHIVE_FAILED;
		} else {
			/* Copy prefix and remainder to appropriate places */
			memcpy(h + USTAR_prefix_offset, pp, p - pp);
			memcpy(h + USTAR_name_offset, p + 1,
			    pp + copy_length - p - 1);
		}
	}

	r = archive_entry_hardlink_l(entry, &p, &copy_length, sconv);
	if (r != 0) {
		if (errno == ENOMEM) {
			archive_set_error(&a->archive, ENOMEM,
			    "Can't allocate memory for Linkname");
			return (ARCHIVE_FATAL);
		}
		archive_set_error(&a->archive,
		    ARCHIVE_ERRNO_FILE_FORMAT,
		    "Can't translate linkname '%s' to %s",
		    p, archive_string_conversion_charset_name(sconv));
		ret = ARCHIVE_WARN;
	}
	if (copy_length > 0)
		mytartype = '1';
	else {
		r = archive_entry_symlink_l(entry, &p, &copy_length, sconv);
		if (r != 0) {
			if (errno == ENOMEM) {
				archive_set_error(&a->archive, ENOMEM,
				    "Can't allocate memory for Linkname");
				return (ARCHIVE_FATAL);
			}
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Can't translate linkname '%s' to %s",
			    p, archive_string_conversion_charset_name(sconv));
			ret = ARCHIVE_WARN;
		}
	}
	if (copy_length > 0) {
		if (copy_length > USTAR_linkname_size) {
			archive_set_error(&a->archive, ENAMETOOLONG,
			    "Link contents too long");
			ret = ARCHIVE_FAILED;
			copy_length = USTAR_linkname_size;
		}
		memcpy(h + USTAR_linkname_offset, p, copy_length);
	}

	r = archive_entry_uname_l(entry, &p, &copy_length, sconv);
	if (r != 0) {
		if (errno == ENOMEM) {
			archive_set_error(&a->archive, ENOMEM,
			    "Can't allocate memory for Uname");
			return (ARCHIVE_FATAL);
		}
		archive_set_error(&a->archive,
		    ARCHIVE_ERRNO_FILE_FORMAT,
		    "Can't translate uname '%s' to %s",
		    p, archive_string_conversion_charset_name(sconv));
		ret = ARCHIVE_WARN;
	}
	if (copy_length > 0) {
		if (copy_length > USTAR_uname_size) {
			if (tartype != 'x') {
				archive_set_error(&a->archive,
				    ARCHIVE_ERRNO_MISC, "Username too long");
				ret = ARCHIVE_FAILED;
			}
			copy_length = USTAR_uname_size;
		}
		memcpy(h + USTAR_uname_offset, p, copy_length);
	}

	r = archive_entry_gname_l(entry, &p, &copy_length, sconv);
	if (r != 0) {
		if (errno == ENOMEM) {
			archive_set_error(&a->archive, ENOMEM,
			    "Can't allocate memory for Gname");
			return (ARCHIVE_FATAL);
		}
		archive_set_error(&a->archive,
		    ARCHIVE_ERRNO_FILE_FORMAT,
		    "Can't translate gname '%s' to %s",
		    p, archive_string_conversion_charset_name(sconv));
		ret = ARCHIVE_WARN;
	}
	if (copy_length > 0) {
		if (strlen(p) > USTAR_gname_size) {
			if (tartype != 'x') {
				archive_set_error(&a->archive,
				    ARCHIVE_ERRNO_MISC, "Group name too long");
				ret = ARCHIVE_FAILED;
			}
			copy_length = USTAR_gname_size;
		}
		memcpy(h + USTAR_gname_offset, p, copy_length);
	}

	if (format_number(archive_entry_mode(entry) & 07777,
	    h + USTAR_mode_offset, USTAR_mode_size, USTAR_mode_max_size, strict)) {
		archive_set_error(&a->archive, ERANGE,
		    "Numeric mode too large");
		ret = ARCHIVE_FAILED;
	}

	if (format_number(archive_entry_uid(entry),
	    h + USTAR_uid_offset, USTAR_uid_size, USTAR_uid_max_size, strict)) {
		archive_set_error(&a->archive, ERANGE,
		    "Numeric user ID too large");
		ret = ARCHIVE_FAILED;
	}

	if (format_number(archive_entry_gid(entry),
	    h + USTAR_gid_offset, USTAR_gid_size, USTAR_gid_max_size, strict)) {
		archive_set_error(&a->archive, ERANGE,
		    "Numeric group ID too large");
		ret = ARCHIVE_FAILED;
	}

	if (format_number(archive_entry_size(entry),
	    h + USTAR_size_offset, USTAR_size_size, USTAR_size_max_size, strict)) {
		archive_set_error(&a->archive, ERANGE,
		    "File size out of range");
		ret = ARCHIVE_FAILED;
	}

	if (format_number(archive_entry_mtime(entry),
	    h + USTAR_mtime_offset, USTAR_mtime_size, USTAR_mtime_max_size, strict)) {
		archive_set_error(&a->archive, ERANGE,
		    "File modification time too large");
		ret = ARCHIVE_FAILED;
	}

	if (archive_entry_filetype(entry) == AE_IFBLK
	    || archive_entry_filetype(entry) == AE_IFCHR) {
		if (format_number(archive_entry_rdevmajor(entry),
		    h + USTAR_rdevmajor_offset, USTAR_rdevmajor_size,
		    USTAR_rdevmajor_max_size, strict)) {
			archive_set_error(&a->archive, ERANGE,
			    "Major device number too large");
			ret = ARCHIVE_FAILED;
		}

		if (format_number(archive_entry_rdevminor(entry),
		    h + USTAR_rdevminor_offset, USTAR_rdevminor_size,
		    USTAR_rdevminor_max_size, strict)) {
			archive_set_error(&a->archive, ERANGE,
			    "Minor device number too large");
			ret = ARCHIVE_FAILED;
		}
	}

	if (tartype >= 0) {
		h[USTAR_typeflag_offset] = tartype;
	} else if (mytartype >= 0) {
		h[USTAR_typeflag_offset] = mytartype;
	} else {
		switch (archive_entry_filetype(entry)) {
		case AE_IFREG: h[USTAR_typeflag_offset] = '0' ; break;
		case AE_IFLNK: h[USTAR_typeflag_offset] = '2' ; break;
		case AE_IFCHR: h[USTAR_typeflag_offset] = '3' ; break;
		case AE_IFBLK: h[USTAR_typeflag_offset] = '4' ; break;
		case AE_IFDIR: h[USTAR_typeflag_offset] = '5' ; break;
		case AE_IFIFO: h[USTAR_typeflag_offset] = '6' ; break;
		default: /* AE_IFSOCK and unknown */
			__archive_write_entry_filetype_unsupported(
			    &a->archive, entry, "ustar");
			ret = ARCHIVE_FAILED;
		}
	}

	checksum = 0;
	for (i = 0; i < 512; i++)
		checksum += 255 & (unsigned int)h[i];
	h[USTAR_checksum_offset + 6] = '\0'; /* Can't be pre-set in the template. */
	/* h[USTAR_checksum_offset + 7] = ' '; */ /* This is pre-set in the template. */
	format_octal(checksum, h + USTAR_checksum_offset, 6);
	return (ret);
}
