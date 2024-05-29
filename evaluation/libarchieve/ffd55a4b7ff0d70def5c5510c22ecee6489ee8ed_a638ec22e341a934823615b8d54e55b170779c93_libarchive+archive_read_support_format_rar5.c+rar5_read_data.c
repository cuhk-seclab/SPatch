static int rar5_read_data(struct archive_read *a, const void **buff,
    size_t *size, int64_t *offset) {
	int ret;
	struct rar5* rar = get_context(a);

	*offset = 0;
	*size = 0;

	if(rar->file.dir > 0) {
		/* Don't process any data if this file entry was declared
		 * as a directory. This is needed, because entries marked as
		 * directory doesn't have any dictionary buffer allocated, so
		 * it's impossible to perform any decompression. */
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Can't decompress an entry marked as a directory");
		return ARCHIVE_FAILED;
	}

	if(!rar->skip_mode && (rar->cstate.last_write_ptr > rar->file.unpacked_size)) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
		    "Unpacker has written too many bytes");
		return ARCHIVE_FATAL;
	}

	ret = use_data(rar, buff, size, offset);
	if(ret == ARCHIVE_OK) {
		return ret;
	}

	if(rar->file.eof == 1) {
		return ARCHIVE_EOF;
	}

	ret = do_unpack(a, rar, buff, size, offset);
	if(ret != ARCHIVE_OK) {
		return ret;
	}

	if(rar->file.bytes_remaining == 0 &&
			rar->cstate.last_write_ptr == rar->file.unpacked_size)
	{
		/* If all bytes of current file were processed, run
		 * finalization.
		 *
		 * Finalization will check checksum against proper values. If
		 * some of the checksums will not match, we'll return an error
		 * value in the last `archive_read_data` call to signal an error
		 * to the user. */

		rar->file.eof = 1;
		return verify_global_checksums(a);
	}

	return ARCHIVE_OK;
}
