static int
client_close_proxy(struct archive_read_filter *self)
{
	return read_client_close_proxy(self->archive);
}
