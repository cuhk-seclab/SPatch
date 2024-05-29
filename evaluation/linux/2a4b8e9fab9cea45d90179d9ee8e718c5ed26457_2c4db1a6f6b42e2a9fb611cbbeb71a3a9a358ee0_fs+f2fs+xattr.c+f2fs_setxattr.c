int f2fs_setxattr(struct inode *inode, int index, const char *name,
				const void *value, size_t size,
				struct page *ipage, int flags)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int err;

	/* this case is only from init_inode_metadata */
	if (ipage)
		return __f2fs_setxattr(inode, index, name, value,
						size, ipage, flags);
	f2fs_balance_fs(sbi, true);

	f2fs_lock_op(sbi);
	/* protect xattr_ver */
	down_write(&F2FS_I(inode)->i_sem);
	err = __f2fs_setxattr(inode, index, name, value, size, ipage, flags);
	up_write(&F2FS_I(inode)->i_sem);
	f2fs_unlock_op(sbi);

	return err;
}
