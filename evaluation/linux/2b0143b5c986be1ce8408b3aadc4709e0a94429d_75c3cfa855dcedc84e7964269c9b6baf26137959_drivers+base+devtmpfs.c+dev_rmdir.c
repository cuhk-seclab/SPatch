static int dev_rmdir(const char *name)
{
	struct path parent;
	struct dentry *dentry;
	int err;

	dentry = kern_path_locked(name, &parent);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);
	if (d_really_is_positive(dentry)) {
		if (d_inode(dentry)->i_private == &thread)
			err = vfs_rmdir(d_inode(parent.dentry), dentry);
		else
			err = -EPERM;
	} else {
		err = -ENOENT;
	}
	dput(dentry);
	mutex_unlock(&d_inode(parent.dentry)->i_mutex);
	path_put(&parent);
	return err;
}
