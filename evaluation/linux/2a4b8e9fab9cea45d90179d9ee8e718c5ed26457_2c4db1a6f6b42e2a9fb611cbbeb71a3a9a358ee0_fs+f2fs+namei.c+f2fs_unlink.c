static int f2fs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dir);
	struct inode *inode = d_inode(dentry);
	struct f2fs_dir_entry *de;
	struct page *page;
	int err = -ENOENT;

	trace_f2fs_unlink_enter(dir, dentry);

	de = f2fs_find_entry(dir, &dentry->d_name, &page);
	if (!de)
		goto fail;

	f2fs_balance_fs(sbi, true);

	f2fs_lock_op(sbi);
	err = acquire_orphan_inode(sbi);
	if (err) {
		f2fs_unlock_op(sbi);
		f2fs_dentry_kunmap(dir, page);
		f2fs_put_page(page, 0);
		goto fail;
	}
	f2fs_delete_entry(de, page, dir, inode);
	f2fs_unlock_op(sbi);

	/* In order to evict this inode, we set it dirty */
	mark_inode_dirty(inode);

	if (IS_DIRSYNC(dir))
		f2fs_sync_fs(sbi->sb, 1);
fail:
	trace_f2fs_unlink_exit(inode, err);
	return err;
}
