int write_checkpoint(struct f2fs_sb_info *sbi, struct cp_control *cpc)
{
	struct f2fs_checkpoint *ckpt = F2FS_CKPT(sbi);
	unsigned long long ckpt_ver;
	int err = 0;

	mutex_lock(&sbi->cp_mutex);

	if (!is_sbi_flag_set(sbi, SBI_IS_DIRTY) &&
		(cpc->reason == CP_FASTBOOT || cpc->reason == CP_SYNC ||
		(cpc->reason == CP_DISCARD && !sbi->discard_blks)))
		goto out;
	if (unlikely(f2fs_cp_error(sbi))) {
		err = -EIO;
		goto out;
	}
	if (f2fs_readonly(sbi->sb)) {
		err = -EROFS;
		goto out;
	}

	trace_f2fs_write_checkpoint(sbi->sb, cpc->reason, "start block_ops");

	err = block_operations(sbi);
	if (err)
		goto out;

	trace_f2fs_write_checkpoint(sbi->sb, cpc->reason, "finish block_ops");

	f2fs_submit_merged_bio(sbi, DATA, WRITE);
	f2fs_submit_merged_bio(sbi, NODE, WRITE);
	f2fs_submit_merged_bio(sbi, META, WRITE);

	/*
	 * update checkpoint pack index
	 * Increase the version number so that
	 * SIT entries and seg summaries are written at correct place
	 */
	ckpt_ver = cur_cp_version(ckpt);
	ckpt->checkpoint_ver = cpu_to_le64(++ckpt_ver);

	/* write cached NAT/SIT entries to NAT/SIT area */
	flush_nat_entries(sbi);
	flush_sit_entries(sbi, cpc);

	/* unlock all the fs_lock[] in do_checkpoint() */
	err = do_checkpoint(sbi, cpc);

	unblock_operations(sbi);
	stat_inc_cp_count(sbi->stat_info);

	if (cpc->reason == CP_RECOVERY)
		f2fs_msg(sbi->sb, KERN_NOTICE,
			"checkpoint: version = %llx", ckpt_ver);

	/* do checkpoint periodically */
	sbi->cp_expires = round_jiffies_up(jiffies + HZ * sbi->cp_interval);
	trace_f2fs_write_checkpoint(sbi->sb, cpc->reason, "finish checkpoint");
out:
	mutex_unlock(&sbi->cp_mutex);
	return err;
}
