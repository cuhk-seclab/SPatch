static int mqueue_unlink(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);

	dir->i_ctime = dir->i_mtime = dir->i_atime = CURRENT_TIME;
	dir->i_size -= DIRENT_SIZE;
	drop_nlink(inode);
	dput(dentry);
	return 0;
}
