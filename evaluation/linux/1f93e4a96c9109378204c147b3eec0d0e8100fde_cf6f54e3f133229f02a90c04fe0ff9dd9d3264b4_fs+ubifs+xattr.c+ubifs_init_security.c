int ubifs_init_security(struct inode *dentry, struct inode *inode,
			const struct qstr *qstr)
{
	int err;

	err = security_inode_init_security(inode, dentry, qstr,
					   &init_xattrs, 0);
	if (err) {
		struct ubifs_info *c = dentry->i_sb->s_fs_info;
		ubifs_err(c, "cannot initialize security for inode %lu, error %d",
			  inode->i_ino, err);
	}
	return err;
}
