void *
kmem_alloc(size_t size, xfs_km_flags_t flags)
{
	int	retries = 0;
	gfp_t	lflags = kmem_flags_convert(flags);
	void	*ptr;

	do {
		ptr = kmalloc(size, lflags);
		if (ptr || (flags & (KM_MAYFAIL|KM_NOSLEEP)))
			return ptr;
		if (!(++retries % 100))
			xfs_err(NULL,
				current->comm, current->pid,
		"possible memory allocation deadlock in %s (mode:0x%x)",
					__func__, lflags);
		congestion_wait(BLK_RW_ASYNC, HZ/50);
	} while (1);
}
