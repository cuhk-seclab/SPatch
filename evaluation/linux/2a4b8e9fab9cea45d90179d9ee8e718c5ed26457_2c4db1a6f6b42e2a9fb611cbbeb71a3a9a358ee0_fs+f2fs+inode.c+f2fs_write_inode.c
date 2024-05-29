int f2fs_write_inode(struct inode *inode, struct writeback_control *wbc)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (inode->i_ino == F2FS_NODE_INO(sbi) ||
			inode->i_ino == F2FS_META_INO(sbi))
		return 0;

	if (!is_inode_flag_set(F2FS_I(inode), FI_DIRTY_INODE))
		return 0;

	/*
	 * We need to balance fs here to prevent from producing dirty node pages
	 * during the urgent cleaning time when runing out of free sections.
	 */
	if (update_inode_page(inode))
		f2fs_balance_fs(sbi, true);
	return 0;
}
