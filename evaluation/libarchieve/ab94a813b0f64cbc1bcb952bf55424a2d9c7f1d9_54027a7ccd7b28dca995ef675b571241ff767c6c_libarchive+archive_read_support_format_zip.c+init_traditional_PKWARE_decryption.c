static int
init_traditional_PKWARE_decryption(struct archive_read *a)
{
	struct zip *zip = (struct zip *)(a->format->data);
	const void *p;
	int retry;
	int r;

	if (zip->tctx_valid)
		return (ARCHIVE_OK);

	/*
	   Read the 12 bytes encryption header stored at
	   the start of the data area.
	 */
#define ENC_HEADER_SIZE	12
	if (0 == (zip->entry->zip_flags & ZIP_LENGTH_AT_END)
	    && zip->entry_bytes_remaining < ENC_HEADER_SIZE) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Truncated Zip encrypted body: only %jd bytes available",
		    (intmax_t)zip->entry_bytes_remaining);
		return (ARCHIVE_FATAL);
	}

	p = __archive_read_ahead(a, ENC_HEADER_SIZE, NULL);
	if (p == NULL) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Truncated ZIP file data");
		return (ARCHIVE_FATAL);
	}

	for (retry = 0;; retry++) {
		const char *passphrase;
		uint8_t crcchk;

		passphrase = __archive_read_next_passphrase(a);
		if (passphrase == NULL) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
			    (retry > 0)?
				"Incorrect passphrase":
				"Passphrase required for this entry");
			return (ARCHIVE_FAILED);
		}

		/*
		 * Initialize ctx for Traditional PKWARE Decryption.
		 */
		r = trad_enc_init(&zip->tctx, passphrase, strlen(passphrase),
			p, ENC_HEADER_SIZE, &crcchk);
		if (r == 0 && crcchk == zip->entry->decdat)
			break;/* The passphrase is OK. */
		if (retry > 10000) {
			/* Avoid infinity loop. */
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
			    "Too many incorrect passphrases");
			return (ARCHIVE_FAILED);
		}
	}

	__archive_read_consume(a, ENC_HEADER_SIZE);
	zip->tctx_valid = 1;
	if (0 == (zip->entry->zip_flags & ZIP_LENGTH_AT_END)) {
	    zip->entry_bytes_remaining -= ENC_HEADER_SIZE;
	}
	/*zip->entry_uncompressed_bytes_read += ENC_HEADER_SIZE;*/
	zip->entry_compressed_bytes_read += ENC_HEADER_SIZE;
	zip->decrypted_bytes_remaining = 0;

	return (zip_alloc_decryption_buffer(a));
#undef ENC_HEADER_SIZE
}
