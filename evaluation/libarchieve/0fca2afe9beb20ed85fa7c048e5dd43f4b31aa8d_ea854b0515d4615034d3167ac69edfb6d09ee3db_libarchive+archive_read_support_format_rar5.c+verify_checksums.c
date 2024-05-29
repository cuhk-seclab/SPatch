static int verify_checksums(struct archive_read* a) {
	int verify_crc;
	struct rar5* rar = get_context(a);

	/* Check checksums only when actually unpacking the data. There's no
	 * need to calculate checksum when we're skipping data in solid archives
	 * (skipping in solid archives is the same thing as unpacking compressed
	 * data and discarding the result). */

	if(!rar->skip_mode) {
		/* Always check checksums if we're not in skip mode */
		verify_crc = 1;
	} else {
		/* We can override the logic above with a compile-time option
		 * NO_CRC_ON_SOLID_SKIP. This option is used during debugging,
		 * and it will check checksums of unpacked data even when
		 * we're skipping it. */

#if defined CHECK_CRC_ON_SOLID_SKIP
		/* Debug case */
		verify_crc = 1;
#else
		/* Normal case */
		verify_crc = 0;
#endif
	}

	if(verify_crc) {
		/* During unpacking, on each unpacked block we're calling the
		 * update_crc() function. Since we are here, the unpacking
		 * process is already over and we can check if calculated
		 * checksum (CRC32 or BLAKE2sp) is the same as what is stored
		 * in the archive. */
		if(rar->file.stored_crc32 > 0) {
			/* Check CRC32 only when the file contains a CRC32
			 * value for this file. */

			if(rar->file.calculated_crc32 !=
			    rar->file.stored_crc32) {
				/* Checksums do not match; the unpacked file
				 * is corrupted. */

				DEBUG_CODE {
					printf("Checksum error: CRC32 "
					    "(was: %08" PRIx32 ", expected: %08" PRIx32 ")\n",
					    rar->file.calculated_crc32,
					    rar->file.stored_crc32);
				}

#ifndef DONT_FAIL_ON_CRC_ERROR
				archive_set_error(&a->archive,
				    ARCHIVE_ERRNO_FILE_FORMAT,
				    "Checksum error: CRC32");
				return ARCHIVE_FATAL;
#endif
			} else {
				DEBUG_CODE {
					printf("Checksum OK: CRC32 "
					    "(%08" PRIx32 "/%08" PRIx32 ")\n",
					    rar->file.stored_crc32,
					    rar->file.calculated_crc32);
				}
			}
		}

		if(rar->file.has_blake2 > 0) {
			/* BLAKE2sp is an optional checksum algorithm that is
			 * added to RARv5 archives when using the `-htb` switch
			 *  during creation of archive.
			 *
			 * We now finalize the hash calculation by calling the
			 * `final` function. This will generate the final hash
			 * value we can use to compare it with the BLAKE2sp
			 * checksum that is stored in the archive.
			 *
			 * The return value of this `final` function is not
			 * very helpful, as it guards only against improper use.
 			 * This is why we're explicitly ignoring it. */

			uint8_t b2_buf[32];
			(void) blake2sp_final(&rar->file.b2state, b2_buf, 32);

			if(memcmp(&rar->file.blake2sp, b2_buf, 32) != 0) {
#ifndef DONT_FAIL_ON_CRC_ERROR
				archive_set_error(&a->archive,
				    ARCHIVE_ERRNO_FILE_FORMAT,
				    "Checksum error: BLAKE2");

				return ARCHIVE_FATAL;
#endif
			}
		}
	}

	/* Finalization for this file has been successfully completed. */
	return ARCHIVE_OK;
}
