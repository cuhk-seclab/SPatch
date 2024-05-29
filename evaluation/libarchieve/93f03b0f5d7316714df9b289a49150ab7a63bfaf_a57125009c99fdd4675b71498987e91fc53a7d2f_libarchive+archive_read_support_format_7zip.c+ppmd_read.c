static Byte
ppmd_read(void *p)
{
	struct archive_read *a = ((IByteIn*)p)->a;
	struct _7zip *zip = (struct _7zip *)(a->format->data);
	Byte b;

	if (zip->ppstream.avail_in <= 0) {
		// Ppmd7_DecodeSymbol might require reading multiple bytes and we are on boundary; last resort to read using __archive_read_ahead.
		ssize_t bytes_avail = 0;
		const uint8_t* data = __archive_read_ahead(a, zip->ppstream.stream_in+1, &bytes_avail);
		if(bytes_avail < zip->ppstream.stream_in+1) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
				"Truncated 7z file data");
			zip->ppstream.overconsumed = 1;
			return (0);
		}
		zip->ppstream.next_in++;
		b = data[zip->ppstream.stream_in];
		zip->ppstream.avail_in--;
		zip->ppstream.total_in++;
		zip->ppstream.stream_in++;
	} else {
		b = *zip->ppstream.next_in++;
		zip->ppstream.avail_in--;
		zip->ppstream.total_in++;
		zip->ppstream.stream_in++;
	}
	return (b);
}
