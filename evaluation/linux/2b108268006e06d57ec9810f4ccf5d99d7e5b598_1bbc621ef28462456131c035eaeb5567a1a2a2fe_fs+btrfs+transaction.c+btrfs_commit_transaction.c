int btrfs_commit_transaction(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root)
{
	struct btrfs_transaction *cur_trans = trans->transaction;
	struct btrfs_transaction *prev_trans = NULL;
	struct btrfs_inode *btree_ino = BTRFS_I(root->fs_info->btree_inode);
	int ret;

	/* Stop the commit early if ->aborted is set */
	if (unlikely(ACCESS_ONCE(cur_trans->aborted))) {
		ret = cur_trans->aborted;
		btrfs_end_transaction(trans, root);
		return ret;
	}

	/* make a pass through all the delayed refs we have so far
	 * any runnings procs may add more while we are here
	 */
	ret = btrfs_run_delayed_refs(trans, root, 0);
	if (ret) {
		btrfs_end_transaction(trans, root);
		return ret;
	}

	btrfs_trans_release_metadata(trans, root);
	trans->block_rsv = NULL;
	if (trans->qgroup_reserved) {
		btrfs_qgroup_free(root, trans->qgroup_reserved);
		trans->qgroup_reserved = 0;
	}

	cur_trans = trans->transaction;

	/*
	 * set the flushing flag so procs in this transaction have to
	 * start sending their work down.
	 */
	cur_trans->delayed_refs.flushing = 1;
	smp_wmb();

	if (!list_empty(&trans->new_bgs))
		btrfs_create_pending_block_groups(trans, root);

	ret = btrfs_run_delayed_refs(trans, root, 0);
	if (ret) {
		btrfs_end_transaction(trans, root);
		return ret;
	}

	spin_lock(&root->fs_info->trans_lock);
	list_splice(&trans->ordered, &cur_trans->pending_ordered);
	if (cur_trans->state >= TRANS_STATE_COMMIT_START) {
		spin_unlock(&root->fs_info->trans_lock);
		atomic_inc(&cur_trans->use_count);
		ret = btrfs_end_transaction(trans, root);

		wait_for_commit(root, cur_trans);

		if (unlikely(cur_trans->aborted))
			ret = cur_trans->aborted;

		btrfs_put_transaction(cur_trans);

		return ret;
	}

	cur_trans->state = TRANS_STATE_COMMIT_START;
	wake_up(&root->fs_info->transaction_blocked_wait);

	if (cur_trans->list.prev != &root->fs_info->trans_list) {
		prev_trans = list_entry(cur_trans->list.prev,
					struct btrfs_transaction, list);
		if (prev_trans->state != TRANS_STATE_COMPLETED) {
			atomic_inc(&prev_trans->use_count);
			spin_unlock(&root->fs_info->trans_lock);

			wait_for_commit(root, prev_trans);

			btrfs_put_transaction(prev_trans);
		} else {
			spin_unlock(&root->fs_info->trans_lock);
		}
	} else {
		spin_unlock(&root->fs_info->trans_lock);
	}

	extwriter_counter_dec(cur_trans, trans->type);

	ret = btrfs_start_delalloc_flush(root->fs_info);
	if (ret)
		goto cleanup_transaction;

	ret = btrfs_run_delayed_items(trans, root);
	if (ret)
		goto cleanup_transaction;

	wait_event(cur_trans->writer_wait,
		   extwriter_counter_read(cur_trans) == 0);

	/* some pending stuffs might be added after the previous flush. */
	ret = btrfs_run_delayed_items(trans, root);
	if (ret)
		goto cleanup_transaction;

	btrfs_wait_delalloc_flush(root->fs_info);

	btrfs_wait_pending_ordered(cur_trans, root->fs_info);

	btrfs_scrub_pause(root);
	/*
	 * Ok now we need to make sure to block out any other joins while we
	 * commit the transaction.  We could have started a join before setting
	 * COMMIT_DOING so make sure to wait for num_writers to == 1 again.
	 */
	spin_lock(&root->fs_info->trans_lock);
	cur_trans->state = TRANS_STATE_COMMIT_DOING;
	spin_unlock(&root->fs_info->trans_lock);
	wait_event(cur_trans->writer_wait,
		   atomic_read(&cur_trans->num_writers) == 1);

	/* ->aborted might be set after the previous check, so check it */
	if (unlikely(ACCESS_ONCE(cur_trans->aborted))) {
		ret = cur_trans->aborted;
		goto scrub_continue;
	}
	/*
	 * the reloc mutex makes sure that we stop
	 * the balancing code from coming in and moving
	 * extents around in the middle of the commit
	 */
	mutex_lock(&root->fs_info->reloc_mutex);

	/*
	 * We needn't worry about the delayed items because we will
	 * deal with them in create_pending_snapshot(), which is the
	 * core function of the snapshot creation.
	 */
	ret = create_pending_snapshots(trans, root->fs_info);
	if (ret) {
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * We insert the dir indexes of the snapshots and update the inode
	 * of the snapshots' parents after the snapshot creation, so there
	 * are some delayed items which are not dealt with. Now deal with
	 * them.
	 *
	 * We needn't worry that this operation will corrupt the snapshots,
	 * because all the tree which are snapshoted will be forced to COW
	 * the nodes and leaves.
	 */
	ret = btrfs_run_delayed_items(trans, root);
	if (ret) {
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	ret = btrfs_run_delayed_refs(trans, root, (unsigned long)-1);
	if (ret) {
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * make sure none of the code above managed to slip in a
	 * delayed item
	 */
	btrfs_assert_delayed_root_empty(root);

	WARN_ON(cur_trans != trans->transaction);

	/* btrfs_commit_tree_roots is responsible for getting the
	 * various roots consistent with each other.  Every pointer
	 * in the tree of tree roots has to point to the most up to date
	 * root for every subvolume and other tree.  So, we have to keep
	 * the tree logging code from jumping in and changing any
	 * of the trees.
	 *
	 * At this point in the commit, there can't be any tree-log
	 * writers, but a little lower down we drop the trans mutex
	 * and let new people in.  By holding the tree_log_mutex
	 * from now until after the super is written, we avoid races
	 * with the tree-log code.
	 */
	mutex_lock(&root->fs_info->tree_log_mutex);

	ret = commit_fs_roots(trans, root);
	if (ret) {
		mutex_unlock(&root->fs_info->tree_log_mutex);
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * Since the transaction is done, we can apply the pending changes
	 * before the next transaction.
	 */
	btrfs_apply_pending_changes(root->fs_info);

	/* commit_fs_roots gets rid of all the tree log roots, it is now
	 * safe to free the root of tree log roots
	 */
	btrfs_free_log_root_tree(trans, root->fs_info);

	ret = commit_cowonly_roots(trans, root);
	if (ret) {
		mutex_unlock(&root->fs_info->tree_log_mutex);
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	/*
	 * The tasks which save the space cache and inode cache may also
	 * update ->aborted, check it.
	 */
	if (unlikely(ACCESS_ONCE(cur_trans->aborted))) {
		ret = cur_trans->aborted;
		mutex_unlock(&root->fs_info->tree_log_mutex);
		mutex_unlock(&root->fs_info->reloc_mutex);
		goto scrub_continue;
	}

	btrfs_prepare_extent_commit(trans, root);

	cur_trans = root->fs_info->running_transaction;

	btrfs_set_root_node(&root->fs_info->tree_root->root_item,
			    root->fs_info->tree_root->node);
	list_add_tail(&root->fs_info->tree_root->dirty_list,
		      &cur_trans->switch_commits);

	btrfs_set_root_node(&root->fs_info->chunk_root->root_item,
			    root->fs_info->chunk_root->node);
	list_add_tail(&root->fs_info->chunk_root->dirty_list,
		      &cur_trans->switch_commits);

	switch_commit_roots(cur_trans, root->fs_info);

	assert_qgroups_uptodate(trans);
	ASSERT(list_empty(&cur_trans->dirty_bgs));
	update_super_roots(root);

	btrfs_set_super_log_root(root->fs_info->super_copy, 0);
	btrfs_set_super_log_root_level(root->fs_info->super_copy, 0);
	memcpy(root->fs_info->super_for_commit, root->fs_info->super_copy,
	       sizeof(*root->fs_info->super_copy));

	btrfs_update_commit_device_size(root->fs_info);
	btrfs_update_commit_device_bytes_used(root, cur_trans);

	clear_bit(BTRFS_INODE_BTREE_LOG1_ERR, &btree_ino->runtime_flags);
	clear_bit(BTRFS_INODE_BTREE_LOG2_ERR, &btree_ino->runtime_flags);

	spin_lock(&root->fs_info->trans_lock);
	cur_trans->state = TRANS_STATE_UNBLOCKED;
	root->fs_info->running_transaction = NULL;
	spin_unlock(&root->fs_info->trans_lock);
	mutex_unlock(&root->fs_info->reloc_mutex);

	wake_up(&root->fs_info->transaction_wait);

	ret = btrfs_write_and_wait_transaction(trans, root);
	if (ret) {
		btrfs_error(root->fs_info, ret,
			    "Error while writing out transaction");
		mutex_unlock(&root->fs_info->tree_log_mutex);
		goto scrub_continue;
	}

	ret = write_ctree_super(trans, root, 0);
	if (ret) {
		mutex_unlock(&root->fs_info->tree_log_mutex);
		goto scrub_continue;
	}

	/*
	 * the super is written, we can safely allow the tree-loggers
	 * to go about their business
	 */
	mutex_unlock(&root->fs_info->tree_log_mutex);

	btrfs_finish_extent_commit(trans, root);

	if (cur_trans->have_free_bgs)
		btrfs_clear_space_info_full(root->fs_info);

	root->fs_info->last_trans_committed = cur_trans->transid;
	/*
	 * We needn't acquire the lock here because there is no other task
	 * which can change it.
	 */
	cur_trans->state = TRANS_STATE_COMPLETED;
	wake_up(&cur_trans->commit_wait);

	spin_lock(&root->fs_info->trans_lock);
	list_del_init(&cur_trans->list);
	spin_unlock(&root->fs_info->trans_lock);

	btrfs_put_transaction(cur_trans);
	btrfs_put_transaction(cur_trans);

	if (trans->type & __TRANS_FREEZABLE)
		sb_end_intwrite(root->fs_info->sb);

	trace_btrfs_transaction_commit(root);

	btrfs_scrub_continue(root);

	if (current->journal_info == trans)
		current->journal_info = NULL;

	kmem_cache_free(btrfs_trans_handle_cachep, trans);

	if (current != root->fs_info->transaction_kthread)
		btrfs_run_delayed_iputs(root);

	return ret;

scrub_continue:
	btrfs_scrub_continue(root);
cleanup_transaction:
	btrfs_trans_release_metadata(trans, root);
	trans->block_rsv = NULL;
	if (trans->qgroup_reserved) {
		btrfs_qgroup_free(root, trans->qgroup_reserved);
		trans->qgroup_reserved = 0;
	}
	btrfs_warn(root->fs_info, "Skipping commit of aborted transaction.");
	if (current->journal_info == trans)
		current->journal_info = NULL;
	cleanup_transaction(trans, root, ret);

	return ret;
}
