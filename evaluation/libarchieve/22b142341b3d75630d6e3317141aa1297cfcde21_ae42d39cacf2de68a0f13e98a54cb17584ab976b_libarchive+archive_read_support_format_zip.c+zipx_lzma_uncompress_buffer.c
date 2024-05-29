static int
zipx_lzma_uncompress_buffer(const char *compressed_buffer,
	size_t compressed_buffer_size,
	char *uncompressed_buffer,
	size_t uncompressed_buffer_size)
{
	int status = ARCHIVE_FATAL;
	// length of 'lzma properties data' in lzma compressed
	// data segment (stream) inside zip archive
	const size_t lzma_params_length = 5;
	// offset of 'lzma properties data' from the beginning of lzma stream
	const size_t lzma_params_offset = 4;
	// end position of 'lzma properties data' in lzma stream
	const size_t lzma_params_end = lzma_params_offset + lzma_params_length;
	if (compressed_buffer == NULL ||
			compressed_buffer_size < lzma_params_end ||
			uncompressed_buffer == NULL)
		return status;

	// prepare header for lzma_alone_decoder to replace zipx header
	// (see comments in 'zipx_lzma_alone_init' for justification)
#pragma pack(push)
#pragma pack(1)
	struct _alone_header
	{
		uint8_t bytes[5]; // lzma_params_length
		uint64_t uncompressed_size;
	} alone_header;
#pragma pack(pop)
	// copy 'lzma properties data' blob
	memcpy(&alone_header.bytes[0], compressed_buffer + lzma_params_offset,
		lzma_params_length);
	alone_header.uncompressed_size = UINT64_MAX;

	// prepare new compressed buffer, see 'zipx_lzma_alone_init' for details
	const size_t lzma_alone_buffer_size =
		compressed_buffer_size - lzma_params_end + sizeof(alone_header);
	unsigned char *lzma_alone_compressed_buffer =
		(unsigned char*) malloc(lzma_alone_buffer_size);
	if (lzma_alone_compressed_buffer == NULL)
		return status;
	// copy lzma_alone header into new buffer
	memcpy(lzma_alone_compressed_buffer, (void*) &alone_header,
		sizeof(alone_header));
	// copy compressed data into new buffer
	memcpy(lzma_alone_compressed_buffer + sizeof(alone_header),
		compressed_buffer + lzma_params_end,
		compressed_buffer_size - lzma_params_end);

	// create and fill in lzma_alone_decoder stream
	lzma_stream stream = LZMA_STREAM_INIT;
	lzma_ret ret = lzma_alone_decoder(&stream, UINT64_MAX);
	if (ret == LZMA_OK)
	{
		stream.next_in = lzma_alone_compressed_buffer;
		stream.avail_in = lzma_alone_buffer_size;
		stream.total_in = 0;
		stream.next_out = (unsigned char*)uncompressed_buffer;
		stream.avail_out = uncompressed_buffer_size;
		stream.total_out = 0;
		ret = lzma_code(&stream, LZMA_RUN);
		if (ret == LZMA_OK || ret == LZMA_STREAM_END)
			status = ARCHIVE_OK;
	}
	lzma_end(&stream);
	free(lzma_alone_compressed_buffer);
	return status;
}
