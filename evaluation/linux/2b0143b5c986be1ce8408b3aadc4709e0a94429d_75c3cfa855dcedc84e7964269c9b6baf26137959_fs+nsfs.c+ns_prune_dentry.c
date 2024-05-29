static void ns_prune_dentry(struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);
	if (inode) {
		struct ns_common *ns = inode->i_private;
		atomic_long_set(&ns->stashed, 0);
	}
}
