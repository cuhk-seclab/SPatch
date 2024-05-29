int btrfs_write_dirty_block_groups(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root)
{
	struct btrfs_block_group_cache *cache;
	struct btrfs_transaction *cur_trans = trans->transaction;
	int ret = 0;
	int should_put;
	struct btrfs_path *path;
	LIST_HEAD(io);
	int num_started = 0;
	int num_waited = 0;
		return 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	/*
	 * We don't need the lock here since we are protected by the transaction
	 * commit.  We want to do the cache_save_setup first and then run the
	 * delayed refs to make sure we have the best chance at doing this all
	 * in one shot.
	 */
	while (!list_empty(&cur_trans->dirty_bgs)) {
		cache = list_first_entry(&cur_trans->dirty_bgs,
					 struct btrfs_block_group_cache,
					 dirty_list);

		/*
		 * this can happen if cache_save_setup re-dirties a block
		 * group that is already under IO.  Just wait for it to
		 * finish and then do it all again
		 */
		if (!list_empty(&cache->io_list)) {
			list_del_init(&cache->io_list);
			btrfs_wait_cache_io(root, trans, cache,
					    &cache->io_ctl, path,
					    cache->key.objectid);
			btrfs_put_block_group(cache);
			num_waited++;
		}

		/*
		 * don't remove from the dirty list until after we've waited
		 * on any pending IO
		 */
		list_del_init(&cache->dirty_list);
		should_put = 1;

		if (cache->disk_cache_state == BTRFS_DC_CLEAR)
			cache_save_setup(cache, trans, path);

		if (!ret)
			ret = btrfs_run_delayed_refs(trans, root, (unsigned long) -1);

		if (!ret && cache->disk_cache_state == BTRFS_DC_SETUP) {
			cache->io_ctl.inode = NULL;
			ret = btrfs_write_out_cache(root, trans, cache, path);
			if (ret == 0 && cache->io_ctl.inode) {
				num_started++;
				should_put = 0;
				list_add_tail(&cache->io_list, &io);
			} else {
				/*
				 * if we failed to write the cache, the
				 * generation will be bad and life goes on
				 */
				ret = 0;
			}
		}
		if (!ret)
			ret = write_one_cache_group(trans, root, path, cache);

		/* if its not on the io list, we need to put the block group */
		if (should_put)
			btrfs_put_block_group(cache);
	}

	while (!list_empty(&io)) {
		cache = list_first_entry(&io, struct btrfs_block_group_cache,
					 io_list);
		list_del_init(&cache->io_list);
		num_waited++;
		btrfs_wait_cache_io(root, trans, cache,
				    &cache->io_ctl, path, cache->key.objectid);
		btrfs_put_block_group(cache);
	}

	btrfs_free_path(path);
	return ret;
}
