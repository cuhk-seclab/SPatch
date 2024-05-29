static int drbd_single_open(struct file *file, int (*show)(struct seq_file *, void *),
		                void *data, struct kref *kref,
				void (*release)(struct kref *))
{
	struct dentry *parent;
	int ret = -ESTALE;

	/* Are we still linked,
	 * or has debugfs_remove() already been called? */
	parent = file->f_path.dentry->d_parent;
	/* not sure if this can happen: */
	if (!parent || d_really_is_negative(parent))
		goto out;
	/* serialize with d_delete() */
	mutex_lock(&d_inode(parent)->i_mutex);
	/* Make sure the object is still alive */
	if (debugfs_positive(file->f_path.dentry)
	&& kref_get_unless_zero(kref))
		ret = 0;
	mutex_unlock(&d_inode(parent)->i_mutex);
	if (!ret) {
		ret = single_open(file, show, data);
		if (ret)
			kref_put(kref, release);
	}
out:
	return ret;
}
