void nfs_file_clear_open_context(struct file *filp)
{
	struct nfs_open_context *ctx = nfs_file_open_context(filp);

	if (ctx) {
		struct inode *inode = d_inode(ctx->dentry);

		/*
		 * We fatal error on write before. Try to writeback
		 * every page again.
		 */
		if (ctx->error < 0)
			invalidate_inode_pages2(inode->i_mapping);
		filp->private_data = NULL;
		spin_lock(&inode->i_lock);
		list_move_tail(&ctx->list, &NFS_I(inode)->open_files);
		spin_unlock(&inode->i_lock);
		put_nfs_open_context_sync(ctx);
	}
}
