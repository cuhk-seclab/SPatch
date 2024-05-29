int open_ctree(struct super_block *sb,
	       struct btrfs_fs_devices *fs_devices,
	       char *options)
{
	u32 sectorsize;
	u32 nodesize;
	u32 stripesize;
	u64 generation;
	u64 features;
	struct btrfs_key location;
	struct buffer_head *bh;
	struct btrfs_super_block *disk_super;
	struct btrfs_fs_info *fs_info = btrfs_sb(sb);
	struct btrfs_root *tree_root;
	struct btrfs_root *chunk_root;
	int ret;
	int err = -EINVAL;
	int num_backups_tried = 0;
	int backup_index = 0;
	int max_active;

	tree_root = fs_info->tree_root = btrfs_alloc_root(fs_info);
	chunk_root = fs_info->chunk_root = btrfs_alloc_root(fs_info);
	if (!tree_root || !chunk_root) {
		err = -ENOMEM;
		goto fail;
	}

	ret = init_srcu_struct(&fs_info->subvol_srcu);
	if (ret) {
		err = ret;
		goto fail;
	}

	ret = setup_bdi(fs_info, &fs_info->bdi);
	if (ret) {
		err = ret;
		goto fail_srcu;
	}

	ret = percpu_counter_init(&fs_info->dirty_metadata_bytes, 0, GFP_KERNEL);
	if (ret) {
		err = ret;
		goto fail_bdi;
	}
	fs_info->dirty_metadata_batch = PAGE_CACHE_SIZE *
					(1 + ilog2(nr_cpu_ids));

	ret = percpu_counter_init(&fs_info->delalloc_bytes, 0, GFP_KERNEL);
	if (ret) {
		err = ret;
		goto fail_dirty_metadata_bytes;
	}

	ret = percpu_counter_init(&fs_info->bio_counter, 0, GFP_KERNEL);
	if (ret) {
		err = ret;
		goto fail_delalloc_bytes;
	}

	fs_info->btree_inode = new_inode(sb);
	if (!fs_info->btree_inode) {
		err = -ENOMEM;
		goto fail_bio_counter;
	}

	mapping_set_gfp_mask(fs_info->btree_inode->i_mapping, GFP_NOFS);

	INIT_RADIX_TREE(&fs_info->fs_roots_radix, GFP_ATOMIC);
	INIT_RADIX_TREE(&fs_info->buffer_radix, GFP_ATOMIC);
	INIT_LIST_HEAD(&fs_info->trans_list);
	INIT_LIST_HEAD(&fs_info->dead_roots);
	INIT_LIST_HEAD(&fs_info->delayed_iputs);
	INIT_LIST_HEAD(&fs_info->delalloc_roots);
	INIT_LIST_HEAD(&fs_info->caching_block_groups);
	spin_lock_init(&fs_info->delalloc_root_lock);
	spin_lock_init(&fs_info->trans_lock);
	spin_lock_init(&fs_info->fs_roots_radix_lock);
	spin_lock_init(&fs_info->delayed_iput_lock);
	spin_lock_init(&fs_info->defrag_inodes_lock);
	spin_lock_init(&fs_info->free_chunk_lock);
	spin_lock_init(&fs_info->tree_mod_seq_lock);
	spin_lock_init(&fs_info->super_lock);
	spin_lock_init(&fs_info->qgroup_op_lock);
	spin_lock_init(&fs_info->buffer_lock);
	spin_lock_init(&fs_info->unused_bgs_lock);
	mutex_init(&fs_info->unused_bg_unpin_mutex);
	rwlock_init(&fs_info->tree_mod_log_lock);
	mutex_init(&fs_info->reloc_mutex);
	mutex_init(&fs_info->delalloc_root_mutex);
	seqlock_init(&fs_info->profiles_lock);

	init_completion(&fs_info->kobj_unregister);
	INIT_LIST_HEAD(&fs_info->dirty_cowonly_roots);
	INIT_LIST_HEAD(&fs_info->space_info);
	INIT_LIST_HEAD(&fs_info->tree_mod_seq_list);
	INIT_LIST_HEAD(&fs_info->unused_bgs);
	btrfs_mapping_init(&fs_info->mapping_tree);
	btrfs_init_block_rsv(&fs_info->global_block_rsv,
			     BTRFS_BLOCK_RSV_GLOBAL);
	btrfs_init_block_rsv(&fs_info->delalloc_block_rsv,
			     BTRFS_BLOCK_RSV_DELALLOC);
	btrfs_init_block_rsv(&fs_info->trans_block_rsv, BTRFS_BLOCK_RSV_TRANS);
	btrfs_init_block_rsv(&fs_info->chunk_block_rsv, BTRFS_BLOCK_RSV_CHUNK);
	btrfs_init_block_rsv(&fs_info->empty_block_rsv, BTRFS_BLOCK_RSV_EMPTY);
	btrfs_init_block_rsv(&fs_info->delayed_block_rsv,
			     BTRFS_BLOCK_RSV_DELOPS);
	atomic_set(&fs_info->nr_async_submits, 0);
	atomic_set(&fs_info->async_delalloc_pages, 0);
	atomic_set(&fs_info->async_submit_draining, 0);
	atomic_set(&fs_info->nr_async_bios, 0);
	atomic_set(&fs_info->defrag_running, 0);
	atomic_set(&fs_info->qgroup_op_seq, 0);
	atomic64_set(&fs_info->tree_mod_seq, 0);
	fs_info->sb = sb;
	fs_info->max_inline = BTRFS_DEFAULT_MAX_INLINE;
	fs_info->metadata_ratio = 0;
	fs_info->defrag_inodes = RB_ROOT;
	fs_info->free_chunk_space = 0;
	fs_info->tree_mod_log = RB_ROOT;
	fs_info->commit_interval = BTRFS_DEFAULT_COMMIT_INTERVAL;
	fs_info->avg_delayed_ref_runtime = NSEC_PER_SEC >> 6; /* div by 64 */
	/* readahead state */
	INIT_RADIX_TREE(&fs_info->reada_tree, GFP_NOFS & ~__GFP_WAIT);
	spin_lock_init(&fs_info->reada_lock);

	fs_info->thread_pool_size = min_t(unsigned long,
					  num_online_cpus() + 2, 8);

	INIT_LIST_HEAD(&fs_info->ordered_roots);
	spin_lock_init(&fs_info->ordered_root_lock);
	fs_info->delayed_root = kmalloc(sizeof(struct btrfs_delayed_root),
					GFP_NOFS);
	if (!fs_info->delayed_root) {
		err = -ENOMEM;
		goto fail_iput;
	}
	btrfs_init_delayed_root(fs_info->delayed_root);

	btrfs_init_scrub(fs_info);
#ifdef CONFIG_BTRFS_FS_CHECK_INTEGRITY
	fs_info->check_integrity_print_mask = 0;
#endif
	btrfs_init_balance(fs_info);
	btrfs_init_async_reclaim_work(&fs_info->async_reclaim_work);

	sb->s_blocksize = 4096;
	sb->s_blocksize_bits = blksize_bits(4096);
	sb->s_bdi = &fs_info->bdi;

	btrfs_init_btree_inode(fs_info, tree_root);

	spin_lock_init(&fs_info->block_group_cache_lock);
	fs_info->block_group_cache_tree = RB_ROOT;
	fs_info->first_logical_byte = (u64)-1;

	extent_io_tree_init(&fs_info->freed_extents[0],
			     fs_info->btree_inode->i_mapping);
	extent_io_tree_init(&fs_info->freed_extents[1],
			     fs_info->btree_inode->i_mapping);
	fs_info->pinned_extents = &fs_info->freed_extents[0];
	fs_info->do_barriers = 1;


	mutex_init(&fs_info->ordered_operations_mutex);
	mutex_init(&fs_info->ordered_extent_flush_mutex);
	mutex_init(&fs_info->tree_log_mutex);
	mutex_init(&fs_info->chunk_mutex);
	mutex_init(&fs_info->transaction_kthread_mutex);
	mutex_init(&fs_info->cleaner_mutex);
	mutex_init(&fs_info->volume_mutex);
	init_rwsem(&fs_info->commit_root_sem);
	init_rwsem(&fs_info->cleanup_work_sem);
	init_rwsem(&fs_info->subvol_sem);
	sema_init(&fs_info->uuid_tree_rescan_sem, 1);

	btrfs_init_dev_replace_locks(fs_info);
	btrfs_init_qgroup(fs_info);

	btrfs_init_free_cluster(&fs_info->meta_alloc_cluster);
	btrfs_init_free_cluster(&fs_info->data_alloc_cluster);

	init_waitqueue_head(&fs_info->transaction_throttle);
	init_waitqueue_head(&fs_info->transaction_wait);
	init_waitqueue_head(&fs_info->transaction_blocked_wait);
	init_waitqueue_head(&fs_info->async_submit_wait);

	INIT_LIST_HEAD(&fs_info->pinned_chunks);

	ret = btrfs_alloc_stripe_hash_table(fs_info);
	if (ret) {
		err = ret;
		goto fail_alloc;
	}

	__setup_root(4096, 4096, 4096, tree_root,
		     fs_info, BTRFS_ROOT_TREE_OBJECTID);

	invalidate_bdev(fs_devices->latest_bdev);

	/*
	 * Read super block and check the signature bytes only
	 */
	bh = btrfs_read_dev_super(fs_devices->latest_bdev);
	if (!bh) {
		err = -EINVAL;
		goto fail_alloc;
	}

	/*
	 * We want to check superblock checksum, the type is stored inside.
	 * Pass the whole disk block of size BTRFS_SUPER_INFO_SIZE (4k).
	 */
	if (btrfs_check_super_csum(bh->b_data)) {
		printk(KERN_ERR "BTRFS: superblock checksum mismatch\n");
		err = -EINVAL;
		goto fail_alloc;
	}

	/*
	 * super_copy is zeroed at allocation time and we never touch the
	 * following bytes up to INFO_SIZE, the checksum is calculated from
	 * the whole block of INFO_SIZE
	 */
	memcpy(fs_info->super_copy, bh->b_data, sizeof(*fs_info->super_copy));
	memcpy(fs_info->super_for_commit, fs_info->super_copy,
	       sizeof(*fs_info->super_for_commit));
	brelse(bh);

	memcpy(fs_info->fsid, fs_info->super_copy->fsid, BTRFS_FSID_SIZE);

	ret = btrfs_check_super_valid(fs_info, sb->s_flags & MS_RDONLY);
	if (ret) {
		printk(KERN_ERR "BTRFS: superblock contains fatal errors\n");
		err = -EINVAL;
		goto fail_alloc;
	}

	disk_super = fs_info->super_copy;
	if (!btrfs_super_root(disk_super))
		goto fail_alloc;

	/* check FS state, whether FS is broken. */
	if (btrfs_super_flags(disk_super) & BTRFS_SUPER_FLAG_ERROR)
		set_bit(BTRFS_FS_STATE_ERROR, &fs_info->fs_state);

	/*
	 * run through our array of backup supers and setup
	 * our ring pointer to the oldest one
	 */
	generation = btrfs_super_generation(disk_super);
	find_oldest_super_backup(fs_info, generation);

	/*
	 * In the long term, we'll store the compression type in the super
	 * block, and it'll be used for per file compression control.
	 */
	fs_info->compress_type = BTRFS_COMPRESS_ZLIB;

	ret = btrfs_parse_options(tree_root, options);
	if (ret) {
		err = ret;
		goto fail_alloc;
	}

	features = btrfs_super_incompat_flags(disk_super) &
		~BTRFS_FEATURE_INCOMPAT_SUPP;
	if (features) {
		printk(KERN_ERR "BTRFS: couldn't mount because of "
		       "unsupported optional features (%Lx).\n",
		       features);
		err = -EINVAL;
		goto fail_alloc;
	}

	/*
	 * Leafsize and nodesize were always equal, this is only a sanity check.
	 */
	if (le32_to_cpu(disk_super->__unused_leafsize) !=
	    btrfs_super_nodesize(disk_super)) {
		printk(KERN_ERR "BTRFS: couldn't mount because metadata "
		       "blocksizes don't match.  node %d leaf %d\n",
		       btrfs_super_nodesize(disk_super),
		       le32_to_cpu(disk_super->__unused_leafsize));
		err = -EINVAL;
		goto fail_alloc;
	}
	if (btrfs_super_nodesize(disk_super) > BTRFS_MAX_METADATA_BLOCKSIZE) {
		printk(KERN_ERR "BTRFS: couldn't mount because metadata "
		       "blocksize (%d) was too large\n",
		       btrfs_super_nodesize(disk_super));
		err = -EINVAL;
		goto fail_alloc;
	}

	features = btrfs_super_incompat_flags(disk_super);
	features |= BTRFS_FEATURE_INCOMPAT_MIXED_BACKREF;
	if (tree_root->fs_info->compress_type == BTRFS_COMPRESS_LZO)
		features |= BTRFS_FEATURE_INCOMPAT_COMPRESS_LZO;

	if (features & BTRFS_FEATURE_INCOMPAT_SKINNY_METADATA)
		printk(KERN_INFO "BTRFS: has skinny extents\n");

	/*
	 * flag our filesystem as having big metadata blocks if
	 * they are bigger than the page size
	 */
	if (btrfs_super_nodesize(disk_super) > PAGE_CACHE_SIZE) {
		if (!(features & BTRFS_FEATURE_INCOMPAT_BIG_METADATA))
			printk(KERN_INFO "BTRFS: flagging fs with big metadata feature\n");
		features |= BTRFS_FEATURE_INCOMPAT_BIG_METADATA;
	}

	nodesize = btrfs_super_nodesize(disk_super);
	sectorsize = btrfs_super_sectorsize(disk_super);
	stripesize = btrfs_super_stripesize(disk_super);
	fs_info->dirty_metadata_batch = nodesize * (1 + ilog2(nr_cpu_ids));
	fs_info->delalloc_batch = sectorsize * 512 * (1 + ilog2(nr_cpu_ids));

	/*
	 * mixed block groups end up with duplicate but slightly offset
	 * extent buffers for the same range.  It leads to corruptions
	 */
	if ((features & BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS) &&
	    (sectorsize != nodesize)) {
		printk(KERN_ERR "BTRFS: unequal leaf/node/sector sizes "
				"are not allowed for mixed block groups on %s\n",
				sb->s_id);
		goto fail_alloc;
	}

	/*
	 * Needn't use the lock because there is no other task which will
	 * update the flag.
	 */
	btrfs_set_super_incompat_flags(disk_super, features);

	features = btrfs_super_compat_ro_flags(disk_super) &
		~BTRFS_FEATURE_COMPAT_RO_SUPP;
	if (!(sb->s_flags & MS_RDONLY) && features) {
		printk(KERN_ERR "BTRFS: couldn't mount RDWR because of "
		       "unsupported option features (%Lx).\n",
		       features);
		err = -EINVAL;
		goto fail_alloc;
	}

	max_active = fs_info->thread_pool_size;

	ret = btrfs_init_workqueues(fs_info, fs_devices);
	if (ret) {
		err = ret;
		goto fail_sb_buffer;
	}

	fs_info->bdi.ra_pages *= btrfs_super_num_devices(disk_super);
	fs_info->bdi.ra_pages = max(fs_info->bdi.ra_pages,
				    4 * 1024 * 1024 / PAGE_CACHE_SIZE);

	tree_root->nodesize = nodesize;
	tree_root->sectorsize = sectorsize;
	tree_root->stripesize = stripesize;

	sb->s_blocksize = sectorsize;
	sb->s_blocksize_bits = blksize_bits(sectorsize);

	if (btrfs_super_magic(disk_super) != BTRFS_MAGIC) {
		printk(KERN_ERR "BTRFS: valid FS not found on %s\n", sb->s_id);
		goto fail_sb_buffer;
	}

	if (sectorsize != PAGE_SIZE) {
		printk(KERN_ERR "BTRFS: incompatible sector size (%lu) "
		       "found on %s\n", (unsigned long)sectorsize, sb->s_id);
		goto fail_sb_buffer;
	}

	mutex_lock(&fs_info->chunk_mutex);
	ret = btrfs_read_sys_array(tree_root);
	mutex_unlock(&fs_info->chunk_mutex);
	if (ret) {
		printk(KERN_ERR "BTRFS: failed to read the system "
		       "array on %s\n", sb->s_id);
		goto fail_sb_buffer;
	}

	generation = btrfs_super_chunk_root_generation(disk_super);

	__setup_root(nodesize, sectorsize, stripesize, chunk_root,
		     fs_info, BTRFS_CHUNK_TREE_OBJECTID);

	chunk_root->node = read_tree_block(chunk_root,
					   btrfs_super_chunk_root(disk_super),
					   generation);
	if (!chunk_root->node ||
	    !test_bit(EXTENT_BUFFER_UPTODATE, &chunk_root->node->bflags)) {
		printk(KERN_ERR "BTRFS: failed to read chunk root on %s\n",
		       sb->s_id);
		goto fail_tree_roots;
	}
	btrfs_set_root_node(&chunk_root->root_item, chunk_root->node);
	chunk_root->commit_root = btrfs_root_node(chunk_root);

	read_extent_buffer(chunk_root->node, fs_info->chunk_tree_uuid,
	   btrfs_header_chunk_tree_uuid(chunk_root->node), BTRFS_UUID_SIZE);

	ret = btrfs_read_chunk_tree(chunk_root);
	if (ret) {
		printk(KERN_ERR "BTRFS: failed to read chunk tree on %s\n",
		       sb->s_id);
		goto fail_tree_roots;
	}

	/*
	 * keep the device that is marked to be the target device for the
	 * dev_replace procedure
	 */
	btrfs_close_extra_devices(fs_devices, 0);

	if (!fs_devices->latest_bdev) {
		printk(KERN_ERR "BTRFS: failed to read devices on %s\n",
		       sb->s_id);
		goto fail_tree_roots;
	}

retry_root_backup:
	generation = btrfs_super_generation(disk_super);

	tree_root->node = read_tree_block(tree_root,
					  btrfs_super_root(disk_super),
					  generation);
	if (!tree_root->node ||
	    !test_bit(EXTENT_BUFFER_UPTODATE, &tree_root->node->bflags)) {
		printk(KERN_WARNING "BTRFS: failed to read tree root on %s\n",
		       sb->s_id);

		goto recovery_tree_root;
	}

	btrfs_set_root_node(&tree_root->root_item, tree_root->node);
	tree_root->commit_root = btrfs_root_node(tree_root);
	btrfs_set_root_refs(&tree_root->root_item, 1);

	ret = btrfs_read_roots(fs_info, tree_root);
	if (ret)
		goto recovery_tree_root;

	fs_info->generation = generation;
	fs_info->last_trans_committed = generation;

	ret = btrfs_recover_balance(fs_info);
	if (ret) {
		printk(KERN_ERR "BTRFS: failed to recover balance\n");
		goto fail_block_groups;
	}

	ret = btrfs_init_dev_stats(fs_info);
	if (ret) {
		printk(KERN_ERR "BTRFS: failed to init dev_stats: %d\n",
		       ret);
		goto fail_block_groups;
	}

	ret = btrfs_init_dev_replace(fs_info);
	if (ret) {
		pr_err("BTRFS: failed to init dev_replace: %d\n", ret);
		goto fail_block_groups;
	}

	btrfs_close_extra_devices(fs_devices, 1);

	ret = btrfs_sysfs_add_one(fs_info);
	if (ret) {
		pr_err("BTRFS: failed to init sysfs interface: %d\n", ret);
		goto fail_block_groups;
	}

	ret = btrfs_init_space_info(fs_info);
	if (ret) {
		printk(KERN_ERR "BTRFS: Failed to initial space info: %d\n", ret);
		goto fail_sysfs;
	}

	ret = btrfs_read_block_groups(fs_info->extent_root);
	if (ret) {
		printk(KERN_ERR "BTRFS: Failed to read block groups: %d\n", ret);
		goto fail_sysfs;
	}
	fs_info->num_tolerated_disk_barrier_failures =
		btrfs_calc_num_tolerated_disk_barrier_failures(fs_info);
	if (fs_info->fs_devices->missing_devices >
	     fs_info->num_tolerated_disk_barrier_failures &&
	    !(sb->s_flags & MS_RDONLY)) {
		printk(KERN_WARNING "BTRFS: "
			"too many missing devices, writeable mount is not allowed\n");
		goto fail_sysfs;
	}

	fs_info->cleaner_kthread = kthread_run(cleaner_kthread, tree_root,
					       "btrfs-cleaner");
	if (IS_ERR(fs_info->cleaner_kthread))
		goto fail_sysfs;

	fs_info->transaction_kthread = kthread_run(transaction_kthread,
						   tree_root,
						   "btrfs-transaction");
	if (IS_ERR(fs_info->transaction_kthread))
		goto fail_cleaner;

	if (!btrfs_test_opt(tree_root, SSD) &&
	    !btrfs_test_opt(tree_root, NOSSD) &&
	    !fs_info->fs_devices->rotating) {
		printk(KERN_INFO "BTRFS: detected SSD devices, enabling SSD "
		       "mode\n");
		btrfs_set_opt(fs_info->mount_opt, SSD);
	}

	/*
	 * Mount does not set all options immediatelly, we can do it now and do
	 * not have to wait for transaction commit
	 */
	btrfs_apply_pending_changes(fs_info);

#ifdef CONFIG_BTRFS_FS_CHECK_INTEGRITY
	if (btrfs_test_opt(tree_root, CHECK_INTEGRITY)) {
		ret = btrfsic_mount(tree_root, fs_devices,
				    btrfs_test_opt(tree_root,
					CHECK_INTEGRITY_INCLUDING_EXTENT_DATA) ?
				    1 : 0,
				    fs_info->check_integrity_print_mask);
		if (ret)
			printk(KERN_WARNING "BTRFS: failed to initialize"
			       " integrity check module %s\n", sb->s_id);
	}
#endif
	ret = btrfs_read_qgroup_config(fs_info);
	if (ret)
		goto fail_trans_kthread;

	/* do not make disk changes in broken FS */
	if (btrfs_super_log_root(disk_super) != 0) {
		ret = btrfs_replay_log(fs_info, fs_devices);
		if (ret) {
			err = ret;
			goto fail_qgroup;
		}
	}

	ret = btrfs_find_orphan_roots(tree_root);
	if (ret)
		goto fail_qgroup;

	if (!(sb->s_flags & MS_RDONLY)) {
		ret = btrfs_cleanup_fs_roots(fs_info);
		if (ret)
			goto fail_qgroup;

		mutex_lock(&fs_info->cleaner_mutex);
		ret = btrfs_recover_relocation(tree_root);
		mutex_unlock(&fs_info->cleaner_mutex);
		if (ret < 0) {
			printk(KERN_WARNING
			       "BTRFS: failed to recover relocation\n");
			err = -EINVAL;
			goto fail_qgroup;
		}
	}

	location.objectid = BTRFS_FS_TREE_OBJECTID;
	location.type = BTRFS_ROOT_ITEM_KEY;
	location.offset = 0;

	fs_info->fs_root = btrfs_read_fs_root_no_name(fs_info, &location);
	if (IS_ERR(fs_info->fs_root)) {
		err = PTR_ERR(fs_info->fs_root);
		goto fail_qgroup;
	}

	if (sb->s_flags & MS_RDONLY)
		return 0;

	down_read(&fs_info->cleanup_work_sem);
	if ((ret = btrfs_orphan_cleanup(fs_info->fs_root)) ||
	    (ret = btrfs_orphan_cleanup(fs_info->tree_root))) {
		up_read(&fs_info->cleanup_work_sem);
		close_ctree(tree_root);
		return ret;
	}
	up_read(&fs_info->cleanup_work_sem);

	ret = btrfs_resume_balance_async(fs_info);
	if (ret) {
		printk(KERN_WARNING "BTRFS: failed to resume balance\n");
		close_ctree(tree_root);
		return ret;
	}

	ret = btrfs_resume_dev_replace_async(fs_info);
	if (ret) {
		pr_warn("BTRFS: failed to resume dev_replace\n");
		close_ctree(tree_root);
		return ret;
	}

	btrfs_qgroup_rescan_resume(fs_info);

	if (!fs_info->uuid_root) {
		pr_info("BTRFS: creating UUID tree\n");
		ret = btrfs_create_uuid_tree(fs_info);
		if (ret) {
			pr_warn("BTRFS: failed to create the UUID tree %d\n",
				ret);
			close_ctree(tree_root);
			return ret;
		}
	} else if (btrfs_test_opt(tree_root, RESCAN_UUID_TREE) ||
		   fs_info->generation !=
				btrfs_super_uuid_tree_generation(disk_super)) {
		pr_info("BTRFS: checking UUID tree\n");
		ret = btrfs_check_uuid_tree(fs_info);
		if (ret) {
			pr_warn("BTRFS: failed to check the UUID tree %d\n",
				ret);
			close_ctree(tree_root);
			return ret;
		}
	} else {
		fs_info->update_uuid_tree_gen = 1;
	}

	fs_info->open = 1;

	return 0;

fail_qgroup:
	btrfs_free_qgroup_config(fs_info);
fail_trans_kthread:
	kthread_stop(fs_info->transaction_kthread);
	btrfs_cleanup_transaction(fs_info->tree_root);
	btrfs_free_fs_roots(fs_info);
fail_cleaner:
	kthread_stop(fs_info->cleaner_kthread);

	/*
	 * make sure we're done with the btree inode before we stop our
	 * kthreads
	 */
	filemap_write_and_wait(fs_info->btree_inode->i_mapping);

fail_sysfs:
	btrfs_sysfs_remove_one(fs_info);

fail_block_groups:
	btrfs_put_block_group_cache(fs_info);
	btrfs_free_block_groups(fs_info);

fail_tree_roots:
	free_root_pointers(fs_info, 1);
	invalidate_inode_pages2(fs_info->btree_inode->i_mapping);

fail_sb_buffer:
	btrfs_stop_all_workers(fs_info);
fail_alloc:
fail_iput:
	btrfs_mapping_tree_free(&fs_info->mapping_tree);

	iput(fs_info->btree_inode);
fail_bio_counter:
	percpu_counter_destroy(&fs_info->bio_counter);
fail_delalloc_bytes:
	percpu_counter_destroy(&fs_info->delalloc_bytes);
fail_dirty_metadata_bytes:
	percpu_counter_destroy(&fs_info->dirty_metadata_bytes);
fail_bdi:
	bdi_destroy(&fs_info->bdi);
fail_srcu:
	cleanup_srcu_struct(&fs_info->subvol_srcu);
fail:
	btrfs_free_stripe_hash_table(fs_info);
	btrfs_close_devices(fs_info->fs_devices);
	return err;

recovery_tree_root:
	if (!btrfs_test_opt(tree_root, RECOVERY))
		goto fail_tree_roots;

	free_root_pointers(fs_info, 0);

	/* don't use the log in recovery mode, it won't be valid */
	btrfs_set_super_log_root(disk_super, 0);

	/* we can't trust the free space cache either */
	btrfs_set_opt(fs_info->mount_opt, CLEAR_CACHE);

	ret = next_root_backup(fs_info, fs_info->super_copy,
			       &num_backups_tried, &backup_index);
	if (ret == -1)
		goto fail_block_groups;
	goto retry_root_backup;
}
