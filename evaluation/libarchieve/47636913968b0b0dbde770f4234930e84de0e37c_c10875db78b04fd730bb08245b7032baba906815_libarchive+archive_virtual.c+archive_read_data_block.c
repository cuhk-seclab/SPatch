int
archive_read_data_block(struct archive *a,
    const void **buff, size_t *s, la_int64_t *o)
{
	return ((a->vtable->archive_read_data_block)(a, buff, s, o));
}
