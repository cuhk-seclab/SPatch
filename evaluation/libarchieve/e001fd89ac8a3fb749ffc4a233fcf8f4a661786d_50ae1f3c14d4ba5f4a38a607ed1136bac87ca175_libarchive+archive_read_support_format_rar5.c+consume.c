static int consume(struct archive_read* a, int64_t how_many) {
	int ret;

	ret = how_many == __archive_read_consume(a, how_many)
		? ARCHIVE_OK
		: ARCHIVE_FATAL;

	return ret;
}
