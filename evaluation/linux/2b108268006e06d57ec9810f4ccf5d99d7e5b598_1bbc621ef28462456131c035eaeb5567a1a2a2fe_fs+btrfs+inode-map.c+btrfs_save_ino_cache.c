int btrfs_save_ino_cache(struct btrfs_root *root,
			 struct btrfs_trans_handle *trans)
{
	struct btrfs_free_space_ctl *ctl = root->free_ino_ctl;
	struct btrfs_path *path;
	struct inode *inode;
	struct btrfs_block_rsv *rsv;
	u64 num_bytes;
	u64 alloc_hint = 0;
	int ret;
	int prealloc;
	bool retry = false;

	/* only fs tree and subvol/snap needs ino cache */
	if (root->root_key.objectid != BTRFS_FS_TREE_OBJECTID &&
	    (root->root_key.objectid < BTRFS_FIRST_FREE_OBJECTID ||
	     root->root_key.objectid > BTRFS_LAST_FREE_OBJECTID))
		return 0;

	/* Don't save inode cache if we are deleting this root */
	if (btrfs_root_refs(&root->root_item) == 0)
		return 0;

	if (!btrfs_test_opt(root, INODE_MAP_CACHE))
		return 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	rsv = trans->block_rsv;
	trans->block_rsv = &root->fs_info->trans_block_rsv;

	num_bytes = trans->bytes_reserved;
	/*
	 * 1 item for inode item insertion if need
	 * 4 items for inode item update (in the worst case)
	 * 1 items for slack space if we need do truncation
	 * 1 item for free space object
	 * 3 items for pre-allocation
	 */
	trans->bytes_reserved = btrfs_calc_trans_metadata_size(root, 10);
	ret = btrfs_block_rsv_add(root, trans->block_rsv,
				  trans->bytes_reserved,
				  BTRFS_RESERVE_NO_FLUSH);
	if (ret)
		goto out;
	trace_btrfs_space_reservation(root->fs_info, "ino_cache",
				      trans->transid, trans->bytes_reserved, 1);
again:
	inode = lookup_free_ino_inode(root, path);
	if (IS_ERR(inode) && (PTR_ERR(inode) != -ENOENT || retry)) {
		ret = PTR_ERR(inode);
		goto out_release;
	}

	if (IS_ERR(inode)) {
		BUG_ON(retry); /* Logic error */
		retry = true;

		ret = create_free_ino_inode(root, trans, path);
		if (ret)
			goto out_release;
		goto again;
	}

	BTRFS_I(inode)->generation = 0;
	ret = btrfs_update_inode(trans, root, inode);
	if (ret) {
		btrfs_abort_transaction(trans, root, ret);
		goto out_put;
	}

	if (i_size_read(inode) > 0) {
		ret = btrfs_truncate_free_space_cache(root, trans, inode);
		if (ret) {
			if (ret != -ENOSPC)
				btrfs_abort_transaction(trans, root, ret);
			goto out_put;
		}
	}

	spin_lock(&root->ino_cache_lock);
	if (root->ino_cache_state != BTRFS_CACHE_FINISHED) {
		ret = -1;
		spin_unlock(&root->ino_cache_lock);
		goto out_put;
	}
	spin_unlock(&root->ino_cache_lock);

	spin_lock(&ctl->tree_lock);
	prealloc = sizeof(struct btrfs_free_space) * ctl->free_extents;
	prealloc = ALIGN(prealloc, PAGE_CACHE_SIZE);
	prealloc += ctl->total_bitmaps * PAGE_CACHE_SIZE;
	spin_unlock(&ctl->tree_lock);

	/* Just to make sure we have enough space */
	prealloc += 8 * PAGE_CACHE_SIZE;

	ret = btrfs_delalloc_reserve_space(inode, prealloc);
	if (ret)
		goto out_put;

	ret = btrfs_prealloc_file_range_trans(inode, trans, 0, 0, prealloc,
					      prealloc, prealloc, &alloc_hint);
	if (ret) {
		btrfs_delalloc_release_space(inode, prealloc);
		goto out_put;
	}
	btrfs_free_reserved_data_space(inode, prealloc);

	ret = btrfs_write_out_ino_cache(root, trans, path, inode);
out_put:
	iput(inode);
out_release:
	trace_btrfs_space_reservation(root->fs_info, "ino_cache",
				      trans->transid, trans->bytes_reserved, 0);
	btrfs_block_rsv_release(root, trans->block_rsv, trans->bytes_reserved);
out:
	trans->block_rsv = rsv;
	trans->bytes_reserved = num_bytes;

	btrfs_free_path(path);
	return ret;
}
