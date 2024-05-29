static void copy_fdtable(struct fdtable *nfdt, struct fdtable *ofdt)
{
	unsigned int cpy, set;

	BUG_ON(nfdt->max_fds < ofdt->max_fds);

	cpy = ofdt->max_fds * sizeof(struct file *);
	set = (nfdt->max_fds - ofdt->max_fds) * sizeof(struct file *);
	memcpy(nfdt->fd, ofdt->fd, cpy);
	memset((char *)nfdt->fd + cpy, 0, set);

	copy_fd_bitmaps(nfdt, ofdt, ofdt->max_fds);
}
