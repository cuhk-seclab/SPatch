static int btrfs_show_options(struct seq_file *seq, struct dentry *dentry)
{
	struct btrfs_fs_info *info = btrfs_sb(dentry->d_sb);
	struct btrfs_root *root = info->tree_root;
	char *compress_type;

	if (btrfs_test_opt(root, DEGRADED))
		seq_puts(seq, ",degraded");
	if (btrfs_test_opt(root, NODATASUM))
		seq_puts(seq, ",nodatasum");
	if (btrfs_test_opt(root, NODATACOW))
		seq_puts(seq, ",nodatacow");
	if (btrfs_test_opt(root, NOBARRIER))
		seq_puts(seq, ",nobarrier");
	if (info->max_inline != BTRFS_DEFAULT_MAX_INLINE)
		seq_printf(seq, ",max_inline=%llu", info->max_inline);
	if (info->alloc_start != 0)
		seq_printf(seq, ",alloc_start=%llu", info->alloc_start);
	if (info->thread_pool_size !=  min_t(unsigned long,
					     num_online_cpus() + 2, 8))
		seq_printf(seq, ",thread_pool=%d", info->thread_pool_size);
	if (btrfs_test_opt(root, COMPRESS)) {
		if (info->compress_type == BTRFS_COMPRESS_ZLIB)
			compress_type = "zlib";
		else
			compress_type = "lzo";
		if (btrfs_test_opt(root, FORCE_COMPRESS))
			seq_printf(seq, ",compress-force=%s", compress_type);
		else
			seq_printf(seq, ",compress=%s", compress_type);
	}
	if (btrfs_test_opt(root, NOSSD))
		seq_puts(seq, ",nossd");
	if (btrfs_test_opt(root, SSD_SPREAD))
		seq_puts(seq, ",ssd_spread");
	else if (btrfs_test_opt(root, SSD))
		seq_puts(seq, ",ssd");
	if (btrfs_test_opt(root, NOTREELOG))
		seq_puts(seq, ",notreelog");
	if (btrfs_test_opt(root, FLUSHONCOMMIT))
		seq_puts(seq, ",flushoncommit");
	if (btrfs_test_opt(root, DISCARD))
		seq_puts(seq, ",discard");
	if (!(root->fs_info->sb->s_flags & MS_POSIXACL))
		seq_puts(seq, ",noacl");
	if (btrfs_test_opt(root, SPACE_CACHE))
		seq_puts(seq, ",space_cache");
	else if (btrfs_test_opt(root, FREE_SPACE_TREE))
		seq_puts(seq, ",space_cache=v2");
	else
		seq_puts(seq, ",nospace_cache");
	if (btrfs_test_opt(root, RESCAN_UUID_TREE))
		seq_puts(seq, ",rescan_uuid_tree");
	if (btrfs_test_opt(root, CLEAR_CACHE))
		seq_puts(seq, ",clear_cache");
	if (btrfs_test_opt(root, USER_SUBVOL_RM_ALLOWED))
		seq_puts(seq, ",user_subvol_rm_allowed");
	if (btrfs_test_opt(root, ENOSPC_DEBUG))
		seq_puts(seq, ",enospc_debug");
	if (btrfs_test_opt(root, AUTO_DEFRAG))
		seq_puts(seq, ",autodefrag");
	if (btrfs_test_opt(root, INODE_MAP_CACHE))
		seq_puts(seq, ",inode_cache");
	if (btrfs_test_opt(root, SKIP_BALANCE))
		seq_puts(seq, ",skip_balance");
	if (btrfs_test_opt(root, RECOVERY))
		seq_puts(seq, ",recovery");
#ifdef CONFIG_BTRFS_FS_CHECK_INTEGRITY
	if (btrfs_test_opt(root, CHECK_INTEGRITY_INCLUDING_EXTENT_DATA))
		seq_puts(seq, ",check_int_data");
	else if (btrfs_test_opt(root, CHECK_INTEGRITY))
		seq_puts(seq, ",check_int");
	if (info->check_integrity_print_mask)
		seq_printf(seq, ",check_int_print_mask=%d",
				info->check_integrity_print_mask);
#endif
	if (info->metadata_ratio)
		seq_printf(seq, ",metadata_ratio=%d",
				info->metadata_ratio);
	if (btrfs_test_opt(root, PANIC_ON_FATAL_ERROR))
		seq_puts(seq, ",fatal_errors=panic");
	if (info->commit_interval != BTRFS_DEFAULT_COMMIT_INTERVAL)
		seq_printf(seq, ",commit=%d", info->commit_interval);
	seq_printf(seq, ",subvolid=%llu",
		  BTRFS_I(d_inode(dentry))->root->root_key.objectid);
	seq_puts(seq, ",subvol=");
	seq_dentry(seq, dentry, " \t\n\\");
	return 0;
}
