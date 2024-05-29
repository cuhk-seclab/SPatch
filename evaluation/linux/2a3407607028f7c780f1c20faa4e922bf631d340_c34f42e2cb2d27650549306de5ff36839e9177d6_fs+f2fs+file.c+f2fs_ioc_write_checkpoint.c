static int f2fs_ioc_write_checkpoint(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct cp_control cpc;
	int err;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	cpc.reason = __get_cp_reason(sbi);

	mutex_lock(&sbi->gc_mutex);
	err = write_checkpoint(sbi, &cpc);
	mutex_unlock(&sbi->gc_mutex);

	return err;
}
