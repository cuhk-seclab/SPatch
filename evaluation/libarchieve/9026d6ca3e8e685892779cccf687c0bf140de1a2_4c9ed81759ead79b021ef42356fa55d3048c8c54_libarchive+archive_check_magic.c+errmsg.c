static void
errmsg(const char *m)
{
	size_t s = strlen(m);
	ssize_t written;

	while (s > 0) {
		written = write(2, m, strlen(m));
		if (written <= 0)
			return;
		m += written;
		s -= written;
	}
}
