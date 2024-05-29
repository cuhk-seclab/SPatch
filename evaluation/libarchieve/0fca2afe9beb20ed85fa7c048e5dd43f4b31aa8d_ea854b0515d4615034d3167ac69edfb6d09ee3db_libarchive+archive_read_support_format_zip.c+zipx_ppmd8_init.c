static int
zipx_ppmd8_init(struct archive_read *a, struct zip *zip)
{
	const void* p;
	uint32_t val;
	uint32_t order;
	uint32_t mem;
	uint32_t restore_method;

	/* Remove previous decompression context if it exists. */
	if(zip->ppmd8_valid) {
		__archive_ppmd8_functions.Ppmd8_Free(&zip->ppmd8);
		zip->ppmd8_valid = 0;
	}

	/* Create a new decompression context. */
	__archive_ppmd8_functions.Ppmd8_Construct(&zip->ppmd8);
	zip->ppmd8_stream_failed = 0;

	/* Setup function pointers required by Ppmd8 decompressor. The
	 * 'ppmd_read' function will feed new bytes to the decompressor,
	 * and will increment the 'zip->zipx_ppmd_read_compressed' counter. */
	zip->ppmd8.Stream.In = &zip->zipx_ppmd_stream;
	zip->zipx_ppmd_stream.a = a;
	zip->zipx_ppmd_stream.Read = &ppmd_read;

	/* Reset number of read bytes to 0. */
	zip->zipx_ppmd_read_compressed = 0;

	/* Read Ppmd8 header (2 bytes). */
	p = __archive_read_ahead(a, 2, NULL);
	if(!p) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Truncated file data in PPMd8 stream");
		return (ARCHIVE_FATAL);
	}
	__archive_read_consume(a, 2);

	/* Decode the stream's compression parameters. */
	val = archive_le16dec(p);
	order = (val & 15) + 1;
	mem = ((val >> 4) & 0xff) + 1;
	restore_method = (val >> 12);

	if(order < 2 || restore_method > 2) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Invalid parameter set in PPMd8 stream (order=%" PRId32 ", "
		    "restore=%" PRId32 ")", order, restore_method);
		return (ARCHIVE_FAILED);
	}

	/* Allocate the memory needed to properly decompress the file. */
	if(!__archive_ppmd8_functions.Ppmd8_Alloc(&zip->ppmd8, mem << 20)) {
		archive_set_error(&a->archive, ENOMEM,
		    "Unable to allocate memory for PPMd8 stream: %" PRId32 " bytes",
		    mem << 20);
		return (ARCHIVE_FATAL);
	}

	/* Signal the cleanup function to release Ppmd8 context in the
	 * cleanup phase. */
	zip->ppmd8_valid = 1;

	/* Perform further Ppmd8 initialization. */
	if(!__archive_ppmd8_functions.Ppmd8_RangeDec_Init(&zip->ppmd8)) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
		    "PPMd8 stream range decoder initialization error");
		return (ARCHIVE_FATAL);
	}

	__archive_ppmd8_functions.Ppmd8_Init(&zip->ppmd8, order,
	    restore_method);

	/* Allocate the buffer that will hold uncompressed data. */
	free(zip->uncompressed_buffer);

	zip->uncompressed_buffer_size = 256 * 1024;
	zip->uncompressed_buffer =
	    (uint8_t*) malloc(zip->uncompressed_buffer_size);

	if(zip->uncompressed_buffer == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "No memory for PPMd8 decompression");
		return ARCHIVE_FATAL;
	}

	/* Ppmd8 initialization is done. */
	zip->decompress_init = 1;

	/* We've already read 2 bytes in the output stream. Additionally,
	 * Ppmd8 initialization code could read some data as well. So we
	 * are advancing the stream by 2 bytes plus whatever number of
	 * bytes Ppmd8 init function used. */
	zip->entry_compressed_bytes_read += 2 + zip->zipx_ppmd_read_compressed;

	return ARCHIVE_OK;
}
