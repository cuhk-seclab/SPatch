int recover_fsync_data(struct f2fs_sb_info *sbi)
{
	struct curseg_info *curseg = CURSEG_I(sbi, CURSEG_WARM_NODE);
	struct list_head inode_list;
	block_t blkaddr;
	int err;
	bool need_writecp = false;

	fsync_entry_slab = f2fs_kmem_cache_create("f2fs_fsync_inode_entry",
			sizeof(struct fsync_inode_entry));
	if (!fsync_entry_slab)
		return -ENOMEM;

	INIT_LIST_HEAD(&inode_list);

	/* prevent checkpoint */
	mutex_lock(&sbi->cp_mutex);

	blkaddr = NEXT_FREE_BLKADDR(sbi, curseg);

	/* step #1: find fsynced inode numbers */
	err = find_fsync_dnodes(sbi, &inode_list);
	if (err)
		goto out;

	if (list_empty(&inode_list))
		goto out;

	need_writecp = true;

	/* step #2: recover data */
	err = recover_data(sbi, &inode_list);
	if (!err)
		f2fs_bug_on(sbi, !list_empty(&inode_list));
out:
	destroy_fsync_dnodes(&inode_list);
	kmem_cache_destroy(fsync_entry_slab);

	/* truncate meta pages to be used by the recovery */
	truncate_inode_pages_range(META_MAPPING(sbi),
			(loff_t)MAIN_BLKADDR(sbi) << PAGE_CACHE_SHIFT, -1);

	if (err) {
		truncate_inode_pages_final(NODE_MAPPING(sbi));
		truncate_inode_pages_final(META_MAPPING(sbi));
	}

	clear_sbi_flag(sbi, SBI_POR_DOING);
	if (err) {
		bool invalidate = false;

		if (discard_next_dnode(sbi, blkaddr))
			invalidate = true;

		/* Flush all the NAT/SIT pages */
		while (get_pages(sbi, F2FS_DIRTY_META))
			sync_meta_pages(sbi, META, LONG_MAX);

		/* invalidate temporary meta page */
		if (invalidate)
			invalidate_mapping_pages(META_MAPPING(sbi),
							blkaddr, blkaddr);

		set_ckpt_flags(sbi->ckpt, CP_ERROR_FLAG);
		mutex_unlock(&sbi->cp_mutex);
	} else if (need_writecp) {
		struct cp_control cpc = {
			.reason = CP_RECOVERY,
		};
		mutex_unlock(&sbi->cp_mutex);
		err = write_checkpoint(sbi, &cpc);
	} else {
		mutex_unlock(&sbi->cp_mutex);
	}
	return err;
}
