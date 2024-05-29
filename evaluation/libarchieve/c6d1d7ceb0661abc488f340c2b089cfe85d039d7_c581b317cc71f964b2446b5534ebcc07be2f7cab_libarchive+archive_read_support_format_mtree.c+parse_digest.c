static int
parse_digest(struct archive_read *a, struct archive_entry *entry,
    const char *digest, int type)
{
	unsigned char digest_buf[64];
	int high, low;
	size_t i, j, len;

	switch (type) {
	case ARCHIVE_ENTRY_DIGEST_MD5:
		len = sizeof(entry->digest.md5);
		break;
	case ARCHIVE_ENTRY_DIGEST_RMD160:
		len = sizeof(entry->digest.rmd160);
		break;
	case ARCHIVE_ENTRY_DIGEST_SHA1:
		len = sizeof(entry->digest.sha1);
		break;
	case ARCHIVE_ENTRY_DIGEST_SHA256:
		len = sizeof(entry->digest.sha256);
		break;
	case ARCHIVE_ENTRY_DIGEST_SHA384:
		len = sizeof(entry->digest.sha384);
		break;
	case ARCHIVE_ENTRY_DIGEST_SHA512:
		len = sizeof(entry->digest.sha512);
		break;
	default:
		archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
			"Internal error: Unknown digest type");
		return ARCHIVE_FATAL;
	}

	if (len > sizeof(digest_buf)) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
			"Internal error: Digest storage too large");
		return ARCHIVE_FATAL;
	}

	len *= 2;

	if (mtree_strnlen(digest, len+1) != len) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
				  "incorrect digest length, ignoring");
		return ARCHIVE_WARN;
	}

	for (i = 0, j = 0; i < len; i += 2, j++) {
		high = parse_hex_nibble(digest[i]);
		low = parse_hex_nibble(digest[i+1]);
		if (high == -1 || low == -1) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
					  "invalid digest data, ignoring");
			return ARCHIVE_WARN;
		}

		digest_buf[j] = high << 4 | low;
	}

	return archive_entry_set_digest(entry, type, digest_buf);
}
