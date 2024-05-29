static int
slurp_central_directory(struct archive_read *a, struct archive_entry* entry,
    struct zip *zip)
{
	ssize_t i;
	unsigned found;
	int64_t correction;
	ssize_t bytes_avail;
	const char *p;

	/*
	 * Find the start of the central directory.  The end-of-CD
	 * record has our starting point, but there are lots of
	 * Zip archives which have had other data prepended to the
	 * file, which makes the recorded offsets all too small.
	 * So we search forward from the specified offset until we
	 * find the real start of the central directory.  Then we
	 * know the correction we need to apply to account for leading
	 * padding.
	 */
	if (__archive_read_seek(a, zip->central_directory_offset_actual, SEEK_SET)
		< 0)
		return ARCHIVE_FATAL;

	found = 0;
	while (!found) {
		if ((p = __archive_read_ahead(a, 20, &bytes_avail)) == NULL)
			return ARCHIVE_FATAL;
		for (found = 0, i = 0; !found && i < bytes_avail - 4;) {
			switch (p[i + 3]) {
			case 'P': i += 3; break;
			case 'K': i += 2; break;
			case 001: i += 1; break;
			case 002:
				if (memcmp(p + i, "PK\001\002", 4) == 0) {
					p += i;
					found = 1;
				} else
					i += 4;
				break;
			case 005: i += 1; break;
			case 006:
				if (memcmp(p + i, "PK\005\006", 4) == 0) {
					p += i;
					found = 1;
				} else if (memcmp(p + i, "PK\006\006", 4) == 0) {
					p += i;
					found = 1;
				} else
					i += 1;
				break;
			default: i += 4; break;
			}
		}
		__archive_read_consume(a, i);
	}
	correction = archive_filter_bytes(&a->archive, 0)
			- zip->central_directory_offset;

	__archive_rb_tree_init(&zip->tree, &rb_ops);
	__archive_rb_tree_init(&zip->tree_rsrc, &rb_rsrc_ops);

	zip->central_directory_entries_total = 0;
	while (1) {
		struct zip_entry *zip_entry;
		size_t filename_length, extra_length, comment_length;
		uint32_t external_attributes;
		const char *name, *r;

		if ((p = __archive_read_ahead(a, 4, NULL)) == NULL)
			return ARCHIVE_FATAL;
		if (memcmp(p, "PK\006\006", 4) == 0
		    || memcmp(p, "PK\005\006", 4) == 0) {
			break;
		} else if (memcmp(p, "PK\001\002", 4) != 0) {
			archive_set_error(&a->archive,
			    -1, "Invalid central directory signature");
			return ARCHIVE_FATAL;
		}
		if ((p = __archive_read_ahead(a, 46, NULL)) == NULL)
			return ARCHIVE_FATAL;

		zip_entry = calloc(1, sizeof(struct zip_entry));
		if (zip_entry == NULL) {
			archive_set_error(&a->archive, ENOMEM,
				"Can't allocate zip entry");
			return ARCHIVE_FATAL;
		}
		zip_entry->next = zip->zip_entries;
		zip_entry->flags |= LA_FROM_CENTRAL_DIRECTORY;
		zip->zip_entries = zip_entry;
		zip->central_directory_entries_total++;

		/* version = p[4]; */
		zip_entry->system = p[5];
		/* version_required = archive_le16dec(p + 6); */
		zip_entry->zip_flags = archive_le16dec(p + 8);
		if (zip_entry->zip_flags
		      & (ZIP_ENCRYPTED | ZIP_STRONG_ENCRYPTED)){
			zip->has_encrypted_entries = 1;
		}
		zip_entry->compression = (char)archive_le16dec(p + 10);
		zip_entry->mtime = zip_time(p + 12);
		zip_entry->crc32 = archive_le32dec(p + 16);
		if (zip_entry->zip_flags & ZIP_LENGTH_AT_END)
			zip_entry->decdat = p[13];
		else
			zip_entry->decdat = p[19];
		zip_entry->compressed_size = archive_le32dec(p + 20);
		zip_entry->uncompressed_size = archive_le32dec(p + 24);
		filename_length = archive_le16dec(p + 28);
		extra_length = archive_le16dec(p + 30);
		comment_length = archive_le16dec(p + 32);
		/* disk_start = archive_le16dec(p + 34);
		 *   Better be zero.
		 * internal_attributes = archive_le16dec(p + 36);
		 *   text bit */
		external_attributes = archive_le32dec(p + 38);
		zip_entry->local_header_offset =
		    archive_le32dec(p + 42) + correction;

		/* If we can't guess the mode, leave it zero here;
		   when we read the local file header we might get
		   more information. */
		if (zip_entry->system == 3) {
			zip_entry->mode = external_attributes >> 16;
		} else if (zip_entry->system == 0) {
			// Interpret MSDOS directory bit
			if (0x10 == (external_attributes & 0x10)) {
				zip_entry->mode = AE_IFDIR | 0775;
			} else {
				zip_entry->mode = AE_IFREG | 0664;
			}
			if (0x01 == (external_attributes & 0x01)) {
				// Read-only bit; strip write permissions
				zip_entry->mode &= 0555;
			}
		} else {
			zip_entry->mode = 0;
		}

		/* We're done with the regular data; get the filename and
		 * extra data. */
		__archive_read_consume(a, 46);
		p = __archive_read_ahead(a, filename_length + extra_length,
			NULL);
		if (p == NULL) {
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Truncated ZIP file header");
			return ARCHIVE_FATAL;
		}
		if (ARCHIVE_OK != process_extra(a, entry, p + filename_length,
		    extra_length, zip_entry)) {
			return ARCHIVE_FATAL;
		}

		/*
		 * Mac resource fork files are stored under the
		 * "__MACOSX/" directory, so we should check if
		 * it is.
		 */
		if (!zip->process_mac_extensions) {
			/* Treat every entry as a regular entry. */
			__archive_rb_tree_insert_node(&zip->tree,
			    &zip_entry->node);
		} else {
			name = p;
			r = rsrc_basename(name, filename_length);
			if (filename_length >= 9 &&
			    strncmp("__MACOSX/", name, 9) == 0) {
				/* If this file is not a resource fork nor
				 * a directory. We should treat it as a non
				 * resource fork file to expose it. */
				if (name[filename_length-1] != '/' &&
				    (r - name < 3 || r[0] != '.' ||
				     r[1] != '_')) {
					__archive_rb_tree_insert_node(
					    &zip->tree, &zip_entry->node);
					/* Expose its parent directories. */
					expose_parent_dirs(zip, name,
					    filename_length);
				} else {
					/* This file is a resource fork file or
					 * a directory. */
					archive_strncpy(&(zip_entry->rsrcname),
					     name, filename_length);
					__archive_rb_tree_insert_node(
					    &zip->tree_rsrc, &zip_entry->node);
				}
			} else {
				/* Generate resource fork name to find its
				 * resource file at zip->tree_rsrc. */
				archive_strcpy(&(zip_entry->rsrcname),
				    "__MACOSX/");
				archive_strncat(&(zip_entry->rsrcname),
				    name, r - name);
				archive_strcat(&(zip_entry->rsrcname), "._");
				archive_strncat(&(zip_entry->rsrcname),
				    name + (r - name),
				    filename_length - (r - name));
				/* Register an entry to RB tree to sort it by
				 * file offset. */
				__archive_rb_tree_insert_node(&zip->tree,
				    &zip_entry->node);
			}
		}

		/* Skip the comment too ... */
		__archive_read_consume(a,
		    filename_length + extra_length + comment_length);
	}

	return ARCHIVE_OK;
}
