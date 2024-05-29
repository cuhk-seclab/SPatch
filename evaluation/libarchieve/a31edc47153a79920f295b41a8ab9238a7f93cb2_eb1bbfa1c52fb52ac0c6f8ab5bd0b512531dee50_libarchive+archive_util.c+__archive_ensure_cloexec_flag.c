void
__archive_ensure_cloexec_flag(int fd)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
	(void)fd; /* UNUSED */
#else
	int flags;

	if (fd >= 0) {
		flags = fcntl(fd, F_GETFD);
		if (flags != -1 && (flags & FD_CLOEXEC) == 0)
			fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
	}
#endif
}
