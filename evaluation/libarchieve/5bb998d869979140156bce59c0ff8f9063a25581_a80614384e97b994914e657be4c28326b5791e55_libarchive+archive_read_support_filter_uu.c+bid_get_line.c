static ssize_t
bid_get_line(struct archive_read_filter *filter,
    const unsigned char **b, ssize_t *avail, ssize_t *ravail,
    ssize_t *nl, size_t* nbytes_read)
{
	ssize_t len;
	int quit;
	
	quit = 0;
	if (*avail == 0) {
		*nl = 0;
		len = 0;
	} else
		len = get_line(*b, *avail, nl);

	/*
	 * Read bytes more while it does not reach the end of line.
	 */
	while (*nl == 0 && len == *avail && !quit &&
	    *nbytes_read < UUENCODE_BID_MAX_READ) {
		ssize_t diff = *ravail - *avail;
		size_t nbytes_req = (*ravail+1023) & ~1023U;
		ssize_t tested;

		/* Increase reading bytes if it is not enough to at least
		 * new two lines. */
		if (nbytes_req < (size_t)*ravail + 160)
			nbytes_req <<= 1;

		*b = __archive_read_filter_ahead(filter, nbytes_req, avail);
		if (*b == NULL) {
			if (*ravail >= *avail)
				return (0);
			/* Reading bytes reaches the end of a stream. */
			*b = __archive_read_filter_ahead(filter, *avail, avail);
			quit = 1;
		}
		*nbytes_read = *avail;
		*ravail = *avail;
		*b += diff;
		*avail -= diff;
		tested = len;/* Skip some bytes we already determinated. */
		len = get_line(*b + tested, *avail - tested, nl);
		if (len >= 0)
			len += tested;
	}
	return (len);
}
