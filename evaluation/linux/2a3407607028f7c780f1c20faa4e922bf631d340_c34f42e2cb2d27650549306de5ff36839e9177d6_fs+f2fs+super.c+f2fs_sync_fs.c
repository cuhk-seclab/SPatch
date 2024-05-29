int f2fs_sync_fs(struct super_block *sb, int sync)
{
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	int err = 0;

	trace_f2fs_sync_fs(sb, sync);

	if (sync) {
		struct cp_control cpc;

		cpc.reason = __get_cp_reason(sbi);

		mutex_lock(&sbi->gc_mutex);
		err = write_checkpoint(sbi, &cpc);
		mutex_unlock(&sbi->gc_mutex);
	} else {
		f2fs_balance_fs(sbi);
	}
	f2fs_trace_ios(NULL, 1);

	return err;
}
