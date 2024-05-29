static long fuse_dev_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	int err = -ENOTTY;

	if (cmd == FUSE_DEV_IOC_CLONE) {
		int oldfd;

		err = -EFAULT;
		if (!get_user(oldfd, (__u32 __user *) arg)) {
			struct file *old = fget(oldfd);

			err = -EINVAL;
			if (old) {
				struct fuse_dev *fud = NULL;

				/*
				 * Check against file->f_op because CUSE
				 * uses the same ioctl handler.
				 */
				if (old->f_op == file->f_op &&
				    old->f_cred->user_ns == file->f_cred->user_ns)
					fud = fuse_get_dev(old);

				if (fud) {
					mutex_lock(&fuse_mutex);
					err = fuse_device_clone(fud->fc, file);
					mutex_unlock(&fuse_mutex);
				}
				fput(old);
			}
		}
	}
	return err;
}
