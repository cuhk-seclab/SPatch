static int
process_extra(struct archive_read *a, const char *p, size_t extra_length, struct zip_entry* zip_entry)
{
	unsigned offset = 0;

	if (extra_length == 0) {
		return ARCHIVE_OK;
	}

	if (extra_length < 4) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Too-small extra data: Need at least 4 bytes, but only found %d bytes", (int)extra_length);
		return ARCHIVE_FAILED;
	}
	while (offset <= extra_length - 4) {
		unsigned short headerid = archive_le16dec(p + offset);
		unsigned short datasize = archive_le16dec(p + offset + 2);

		offset += 4;
		if (offset + datasize > extra_length) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
			    "Extra data overflow: Need %d bytes but only found %d bytes",
			    (int)datasize, (int)(extra_length - offset));
			return ARCHIVE_FAILED;
		}
#ifdef DEBUG
		fprintf(stderr, "Header id 0x%04x, length %d\n",
		    headerid, datasize);
#endif
		switch (headerid) {
		case 0x0001:
			/* Zip64 extended information extra field. */
			zip_entry->flags |= LA_USED_ZIP64;
			if (zip_entry->uncompressed_size == 0xffffffff) {
				uint64_t t = 0;
				if (datasize < 8
				    || (t = archive_le64dec(p + offset)) > INT64_MAX) {
					archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
					    "Malformed 64-bit uncompressed size");
					return ARCHIVE_FAILED;
				}
				zip_entry->uncompressed_size = t;
				offset += 8;
				datasize -= 8;
			}
			if (zip_entry->compressed_size == 0xffffffff) {
				uint64_t t = 0;
				if (datasize < 8
				    || (t = archive_le64dec(p + offset)) > INT64_MAX) {
					archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
					    "Malformed 64-bit compressed size");
					return ARCHIVE_FAILED;
				}
				zip_entry->compressed_size = t;
				offset += 8;
				datasize -= 8;
			}
			if (zip_entry->local_header_offset == 0xffffffff) {
				uint64_t t = 0;
				if (datasize < 8
				    || (t = archive_le64dec(p + offset)) > INT64_MAX) {
					archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
					    "Malformed 64-bit local header offset");
					return ARCHIVE_FAILED;
				}
				zip_entry->local_header_offset = t;
				offset += 8;
				datasize -= 8;
			}
			/* archive_le32dec(p + offset) gives disk
			 * on which file starts, but we don't handle
			 * multi-volume Zip files. */
			break;
#ifdef DEBUG
		case 0x0017:
		{
			/* Strong encryption field. */
			if (archive_le16dec(p + offset) == 2) {
				unsigned algId =
					archive_le16dec(p + offset + 2);
				unsigned bitLen =
					archive_le16dec(p + offset + 4);
				int	 flags =
					archive_le16dec(p + offset + 6);
				fprintf(stderr, "algId=0x%04x, bitLen=%u, "
				    "flgas=%d\n", algId, bitLen,flags);
			}
			break;
		}
#endif
		case 0x5455:
		{
			/* Extended time field "UT". */
			if (datasize == 0) {
				archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
				    "Incomplete extended time field");
				return ARCHIVE_FAILED;
			}
			int flags = p[offset];
			offset++;
			datasize--;
			/* Flag bits indicate which dates are present. */
			if (flags & 0x01)
			{
#ifdef DEBUG
				fprintf(stderr, "mtime: %lld -> %d\n",
				    (long long)zip_entry->mtime,
				    archive_le32dec(p + offset));
#endif
				if (datasize < 4)
					break;
				zip_entry->mtime = archive_le32dec(p + offset);
				offset += 4;
				datasize -= 4;
			}
			if (flags & 0x02)
			{
				if (datasize < 4)
					break;
				zip_entry->atime = archive_le32dec(p + offset);
				offset += 4;
				datasize -= 4;
			}
			if (flags & 0x04)
			{
				if (datasize < 4)
					break;
				zip_entry->ctime = archive_le32dec(p + offset);
				offset += 4;
				datasize -= 4;
			}
			break;
		}
		case 0x5855:
		{
			/* Info-ZIP Unix Extra Field (old version) "UX". */
			if (datasize >= 8) {
				zip_entry->atime = archive_le32dec(p + offset);
				zip_entry->mtime =
				    archive_le32dec(p + offset + 4);
			}
			if (datasize >= 12) {
				zip_entry->uid =
				    archive_le16dec(p + offset + 8);
				zip_entry->gid =
				    archive_le16dec(p + offset + 10);
			}
			break;
		}
		case 0x6c78:
		{
			/* Experimental 'xl' field */
			/*
			 * Introduced Dec 2013 to provide a way to
			 * include external file attributes (and other
			 * fields that ordinarily appear only in
			 * central directory) in local file header.
			 * This provides file type and permission
			 * information necessary to support full
			 * streaming extraction.  Currently being
			 * discussed with other Zip developers
			 * ... subject to change.
			 *
			 * Format:
			 *  The field starts with a bitmap that specifies
			 *  which additional fields are included.  The
			 *  bitmap is variable length and can be extended in
			 *  the future.
			 *
			 *  n bytes - feature bitmap: first byte has low-order
			 *    7 bits.  If high-order bit is set, a subsequent
			 *    byte holds the next 7 bits, etc.
			 *
			 *  if bitmap & 1, 2 byte "version made by"
			 *  if bitmap & 2, 2 byte "internal file attributes"
			 *  if bitmap & 4, 4 byte "external file attributes"
			 *  if bitmap & 8, 2 byte comment length + n byte comment
			 */
			int bitmap, bitmap_last;

			if (datasize < 1)
				break;
			bitmap_last = bitmap = 0xff & p[offset];
			offset += 1;
			datasize -= 1;

			/* We only support first 7 bits of bitmap; skip rest. */
			while ((bitmap_last & 0x80) != 0
			    && datasize >= 1) {
				bitmap_last = p[offset];
				offset += 1;
				datasize -= 1;
			}

			if (bitmap & 1) {
				/* 2 byte "version made by" */
				if (datasize < 2)
					break;
				zip_entry->system
				    = archive_le16dec(p + offset) >> 8;
				offset += 2;
				datasize -= 2;
			}
			if (bitmap & 2) {
				/* 2 byte "internal file attributes" */
				uint32_t internal_attributes;
				if (datasize < 2)
					break;
				internal_attributes
				    = archive_le16dec(p + offset);
				/* Not used by libarchive at present. */
				(void)internal_attributes; /* UNUSED */
				offset += 2;
				datasize -= 2;
			}
			if (bitmap & 4) {
				/* 4 byte "external file attributes" */
				uint32_t external_attributes;
				if (datasize < 4)
					break;
				external_attributes
				    = archive_le32dec(p + offset);
				if (zip_entry->system == 3) {
					zip_entry->mode
					    = external_attributes >> 16;
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
				offset += 4;
				datasize -= 4;
			}
			if (bitmap & 8) {
				/* 2 byte comment length + comment */
				uint32_t comment_length;
				if (datasize < 2)
					break;
				comment_length
				    = archive_le16dec(p + offset);
				offset += 2;
				datasize -= 2;

				if (datasize < comment_length)
					break;
				/* Comment is not supported by libarchive */
				offset += comment_length;
				datasize -= comment_length;
			}
			break;
		}
		case 0x7855:
			/* Info-ZIP Unix Extra Field (type 2) "Ux". */
#ifdef DEBUG
			fprintf(stderr, "uid %d gid %d\n",
			    archive_le16dec(p + offset),
			    archive_le16dec(p + offset + 2));
#endif
			if (datasize >= 2)
				zip_entry->uid = archive_le16dec(p + offset);
			if (datasize >= 4)
				zip_entry->gid =
				    archive_le16dec(p + offset + 2);
			break;
		case 0x7875:
		{
			/* Info-Zip Unix Extra Field (type 3) "ux". */
			int uidsize = 0, gidsize = 0;

			/* TODO: support arbitrary uidsize/gidsize. */
			if (datasize >= 1 && p[offset] == 1) {/* version=1 */
				if (datasize >= 4) {
					/* get a uid size. */
					uidsize = 0xff & (int)p[offset+1];
					if (uidsize == 2)
						zip_entry->uid =
						    archive_le16dec(
						        p + offset + 2);
					else if (uidsize == 4 && datasize >= 6)
						zip_entry->uid =
						    archive_le32dec(
						        p + offset + 2);
				}
				if (datasize >= (2 + uidsize + 3)) {
					/* get a gid size. */
					gidsize = 0xff & (int)p[offset+2+uidsize];
					if (gidsize == 2)
						zip_entry->gid =
						    archive_le16dec(
						        p+offset+2+uidsize+1);
					else if (gidsize == 4 &&
					    datasize >= (2 + uidsize + 5))
						zip_entry->gid =
						    archive_le32dec(
						        p+offset+2+uidsize+1);
				}
			}
			break;
		}
		case 0x9901:
			/* WinZip AES extra data field. */
			if (datasize < 6) {
				archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
				    "Incomplete AES field");
				return ARCHIVE_FAILED;
			}
			if (p[offset + 2] == 'A' && p[offset + 3] == 'E') {
				/* Vendor version. */
				zip_entry->aes_extra.vendor =
				    archive_le16dec(p + offset);
				/* AES encryption strength. */
				zip_entry->aes_extra.strength = p[offset + 4];
				/* Actual compression method. */
				zip_entry->aes_extra.compression =
				    p[offset + 5];
			}
			break;
		default:
			break;
		}
		offset += datasize;
	}
	if (offset != extra_length) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Malformed extra data: Consumed %d bytes of %d bytes",
		    (int)offset, (int)extra_length);
		return ARCHIVE_FAILED;
	}
	return ARCHIVE_OK;
}
