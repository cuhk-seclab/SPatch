void btrfs_delete_unused_bgs(struct btrfs_fs_info *fs_info)
{
	struct btrfs_block_group_cache *block_group;
	struct btrfs_space_info *space_info;
	struct btrfs_root *root = fs_info->extent_root;
	struct btrfs_trans_handle *trans;
	int ret = 0;

	if (!fs_info->open)
		return;

	spin_lock(&fs_info->unused_bgs_lock);
	while (!list_empty(&fs_info->unused_bgs)) {
		u64 start, end;
		int trimming;

		block_group = list_first_entry(&fs_info->unused_bgs,
					       struct btrfs_block_group_cache,
					       bg_list);
		list_del_init(&block_group->bg_list);

		space_info = block_group->space_info;

		if (ret || btrfs_mixed_space_info(space_info)) {
			btrfs_put_block_group(block_group);
			continue;
		}
		spin_unlock(&fs_info->unused_bgs_lock);

		mutex_lock(&root->fs_info->delete_unused_bgs_mutex);

		/* Don't want to race with allocators so take the groups_sem */
		down_write(&space_info->groups_sem);
		spin_lock(&block_group->lock);
		if (block_group->reserved ||
		    btrfs_block_group_used(&block_group->item) ||
		    block_group->ro ||
		    list_is_singular(&block_group->list)) {
			/*
			 * We want to bail if we made new allocations or have
			 * outstanding allocations in this block group.  We do
			 * the ro check in case balance is currently acting on
			 * this block group.
			 */
			spin_unlock(&block_group->lock);
			up_write(&space_info->groups_sem);
			goto next;
		}
		spin_unlock(&block_group->lock);

		/* We don't want to force the issue, only flip if it's ok. */
		ret = inc_block_group_ro(block_group, 0);
		up_write(&space_info->groups_sem);
		if (ret < 0) {
			ret = 0;
			goto next;
		}

		/*
		 * Want to do this before we do anything else so we can recover
		 * properly if we fail to join the transaction.
		 */
		/* 1 for btrfs_orphan_reserve_metadata() */
		trans = btrfs_start_transaction(root, 1);
		if (IS_ERR(trans)) {
			btrfs_dec_block_group_ro(root, block_group);
			ret = PTR_ERR(trans);
			goto next;
		}

		/*
		 * We could have pending pinned extents for this block group,
		 * just delete them, we don't care about them anymore.
		 */
		start = block_group->key.objectid;
		end = start + block_group->key.offset - 1;
		/*
		 * Hold the unused_bg_unpin_mutex lock to avoid racing with
		 * btrfs_finish_extent_commit(). If we are at transaction N,
		 * another task might be running finish_extent_commit() for the
		 * previous transaction N - 1, and have seen a range belonging
		 * to the block group in freed_extents[] before we were able to
		 * clear the whole block group range from freed_extents[]. This
		 * means that task can lookup for the block group after we
		 * unpinned it from freed_extents[] and removed it, leading to
		 * a BUG_ON() at btrfs_unpin_extent_range().
		 */
		mutex_lock(&fs_info->unused_bg_unpin_mutex);
		ret = clear_extent_bits(&fs_info->freed_extents[0], start, end,
				  EXTENT_DIRTY, GFP_NOFS);
		if (ret) {
			mutex_unlock(&fs_info->unused_bg_unpin_mutex);
			btrfs_dec_block_group_ro(root, block_group);
			goto end_trans;
		}
		ret = clear_extent_bits(&fs_info->freed_extents[1], start, end,
				  EXTENT_DIRTY, GFP_NOFS);
		if (ret) {
			mutex_unlock(&fs_info->unused_bg_unpin_mutex);
			btrfs_dec_block_group_ro(root, block_group);
			goto end_trans;
		}
		mutex_unlock(&fs_info->unused_bg_unpin_mutex);

		/* Reset pinned so btrfs_put_block_group doesn't complain */
		spin_lock(&space_info->lock);
		spin_lock(&block_group->lock);

		space_info->bytes_pinned -= block_group->pinned;
		space_info->bytes_readonly += block_group->pinned;
		percpu_counter_add(&space_info->total_bytes_pinned,
				   -block_group->pinned);
		block_group->pinned = 0;

		spin_unlock(&block_group->lock);
		spin_unlock(&space_info->lock);

		/* DISCARD can flip during remount */
		trimming = btrfs_test_opt(root, DISCARD);

		/* Implicit trim during transaction commit. */
		if (trimming)
			btrfs_get_block_group_trimming(block_group);

		/*
		 * Btrfs_remove_chunk will abort the transaction if things go
		 * horribly wrong.
		 */
		ret = btrfs_remove_chunk(trans, root,
					 block_group->key.objectid);

		if (ret) {
			if (trimming)
				btrfs_put_block_group_trimming(block_group);
			goto end_trans;
		}

		/*
		 * If we're not mounted with -odiscard, we can just forget
		 * about this block group. Otherwise we'll need to wait
		 * until transaction commit to do the actual discard.
		 */
		if (trimming) {
			WARN_ON(!list_empty(&block_group->bg_list));
			spin_lock(&trans->transaction->deleted_bgs_lock);
			list_move(&block_group->bg_list,
				  &trans->transaction->deleted_bgs);
			spin_unlock(&trans->transaction->deleted_bgs_lock);
			btrfs_get_block_group(block_group);
		}
end_trans:
		btrfs_end_transaction(trans, root);
next:
		mutex_unlock(&root->fs_info->delete_unused_bgs_mutex);
		btrfs_put_block_group(block_group);
		spin_lock(&fs_info->unused_bgs_lock);
	}
	spin_unlock(&fs_info->unused_bgs_lock);
}
