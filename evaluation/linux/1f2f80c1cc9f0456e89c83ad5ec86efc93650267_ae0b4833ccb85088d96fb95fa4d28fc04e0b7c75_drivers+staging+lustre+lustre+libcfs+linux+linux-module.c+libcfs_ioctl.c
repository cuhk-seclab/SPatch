static long libcfs_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct cfs_psdev_file	 pfile;
	int    rc = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	if (_IOC_TYPE(cmd) != IOC_LIBCFS_TYPE ||
	    _IOC_NR(cmd) < IOC_LIBCFS_MIN_NR  ||
	    _IOC_NR(cmd) > IOC_LIBCFS_MAX_NR) {
		CDEBUG(D_IOCTL, "invalid ioctl ( type %d, nr %d, size %d )\n",
		       _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_SIZE(cmd));
		return -EINVAL;
	}

	/* Handle platform-dependent IOC requests */
	switch (cmd) {
	case IOC_LIBCFS_PANIC:
		if (!capable(CFS_CAP_SYS_BOOT))
			return -EPERM;
		panic("debugctl-invoked panic");
		return 0;
	}

	if (libcfs_psdev_ops.p_ioctl)
		rc = libcfs_psdev_ops.p_ioctl(&pfile, cmd, (void __user *)arg);
	else
		rc = -EPERM;
	return rc;
}
