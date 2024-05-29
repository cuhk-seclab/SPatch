int btrfs_set_block_group_ro(struct btrfs_root *root,
			     struct btrfs_block_group_cache *cache)

{
	struct btrfs_trans_handle *trans;
	u64 alloc_flags;
	int ret;

	BUG_ON(cache->ro);

again:
	trans = btrfs_join_transaction(root);
	if (IS_ERR(trans))
		return PTR_ERR(trans);

	/*
	 * we're not allowed to set block groups readonly after the dirty
	 * block groups cache has started writing.  If it already started,
	 * back off and let this transaction commit
	 */
	mutex_lock(&root->fs_info->ro_block_group_mutex);
	if (trans->transaction->dirty_bg_run) {
		u64 transid = trans->transid;

		mutex_unlock(&root->fs_info->ro_block_group_mutex);
		btrfs_end_transaction(trans, root);

		ret = btrfs_wait_for_commit(root, transid);
		if (ret)
			return ret;
		goto again;
	}


	ret = set_block_group_ro(cache, 0);
	if (!ret)
		goto out;
	alloc_flags = get_alloc_profile(root, cache->space_info->flags);
	ret = do_chunk_alloc(trans, root, alloc_flags,
			     CHUNK_ALLOC_FORCE);
	if (ret < 0)
		goto out;
	ret = set_block_group_ro(cache, 0);
out:
	if (cache->flags & BTRFS_BLOCK_GROUP_SYSTEM) {
		alloc_flags = update_block_group_flags(root, cache->flags);
		lock_chunks(root->fs_info->chunk_root);
		check_system_chunk(trans, root, alloc_flags);
		unlock_chunks(root->fs_info->chunk_root);
	}
	mutex_unlock(&root->fs_info->ro_block_group_mutex);

	btrfs_end_transaction(trans, root);
	return ret;
}
