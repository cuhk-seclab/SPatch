int btrfs_wait_cache_io(struct btrfs_root *root,
			struct btrfs_trans_handle *trans,
			struct btrfs_block_group_cache *block_group,
			struct btrfs_io_ctl *io_ctl,
			struct btrfs_path *path, u64 offset)
{
	int ret;
	struct inode *inode = io_ctl->inode;

	if (!inode)
		return 0;

	root = root->fs_info->tree_root;

	/* Flush the dirty pages in the cache file. */
	ret = flush_dirty_cache(inode);
	if (ret)
		goto out;

	/* Update the cache item to tell everyone this cache file is valid. */
	ret = update_cache_item(trans, root, inode, path, offset,
				io_ctl->entries, io_ctl->bitmaps);
out:
	io_ctl_free(io_ctl);
	if (ret) {
		invalidate_inode_pages2(inode->i_mapping);
		BTRFS_I(inode)->generation = 0;
		if (block_group) {
#ifdef DEBUG
			btrfs_err(root->fs_info,
				"failed to write free space cache for block group %llu",
				block_group->key.objectid);
#endif
		}
	}
	btrfs_update_inode(trans, root, inode);

	if (block_group) {
		/* the dirty list is protected by the dirty_bgs_lock */
		spin_lock(&trans->transaction->dirty_bgs_lock);

		/* the disk_cache_state is protected by the block group lock */
		spin_lock(&block_group->lock);

		/*
		 * only mark this as written if we didn't get put back on
		 * the dirty list while waiting for IO.   Otherwise our
		 * cache state won't be right, and we won't get written again
		 */
		if (!ret && list_empty(&block_group->dirty_list))
			block_group->disk_cache_state = BTRFS_DC_WRITTEN;
		else if (ret)
			block_group->disk_cache_state = BTRFS_DC_ERROR;

		spin_unlock(&block_group->lock);
		spin_unlock(&trans->transaction->dirty_bgs_lock);
		io_ctl->inode = NULL;
		iput(inode);
	}

	return ret;

}
