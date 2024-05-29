static int do_unpack(struct archive_read* a, struct rar5* rar,
    const void** buf, size_t* size, int64_t* offset)
{
	enum COMPRESSION_METHOD {
		STORE = 0, FASTEST = 1, FAST = 2, NORMAL = 3, GOOD = 4,
		BEST = 5
	};

	if(rar->file.service > 0) {
		return do_unstore_file(a, rar, buf, size, offset);
	} else {
		switch(rar->cstate.method) {
			case STORE:
				return do_unstore_file(a, rar, buf, size,
				    offset);
			case FASTEST:
				/* fallthrough */
			case FAST:
				/* fallthrough */
			case NORMAL:
				/* fallthrough */
			case GOOD:
				/* fallthrough */
			case BEST:
				 * caller (like archive_read_data_into_fd) may treat zero-size
				 * as a sparse file block, we need to update the offset
				 * accordingly. At this point the decoder doesn't have any
				 * pending uncompressed data blocks, so the current position in
				 * the output file should be last_write_ptr. */
				return uncompress_file(a);
			default:
				archive_set_error(&a->archive,
				    ARCHIVE_ERRNO_FILE_FORMAT,
				    "Compression method not supported: 0x%x",
				    rar->cstate.method);

				return ARCHIVE_FATAL;
		}
	}

#if !defined WIN32
	/* Not reached. */
	return ARCHIVE_OK;
#endif
}
