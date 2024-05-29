static char *pipefs_dname(struct dentry *dentry, char *buffer, int buflen)
{
	return dynamic_dname(dentry, buffer, buflen, "pipe:[%lu]",
				d_inode(dentry)->i_ino);
}
