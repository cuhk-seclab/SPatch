static int create_file(const char *name, umode_t mode,
		       struct dentry *parent, struct dentry **dentry,
		       const struct file_operations *fops, void *data)
{
	int error;

	mutex_lock(&parent->d_inode->i_mutex);
	*dentry = lookup_one_len(name, parent, strlen(name));
	if (!IS_ERR(*dentry))
		error = qibfs_mknod(d_inode(parent), *dentry,
				    mode, fops, data);
	else
		error = PTR_ERR(*dentry);
	mutex_unlock(&d_inode(parent)->i_mutex);

	return error;
}
