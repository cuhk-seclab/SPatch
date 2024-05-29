static Byte
ppmd_read(void* p) {
	/* Get the handle to current decompression context. */
	struct archive_read *a = ((IByteIn*)p)->a;
	struct zip *zip = (struct zip*) a->format->data;
	ssize_t bytes_avail = 0;

	/* Fetch next byte. */
	const uint8_t* data = __archive_read_ahead(a, 1, &bytes_avail);
	if(bytes_avail < 1) {
		zip->ppmd8_stream_failed = 1;
		return 0;
	}

	__archive_read_consume(a, 1);

	/* Increment the counter. */
	++zip->zipx_ppmd_read_compressed;

	/* Return the next compressed byte. */
	return data[0];
}
