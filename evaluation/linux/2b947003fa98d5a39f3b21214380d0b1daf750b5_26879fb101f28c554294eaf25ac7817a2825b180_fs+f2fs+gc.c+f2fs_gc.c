int f2fs_gc(struct f2fs_sb_info *sbi, bool sync)
{
	unsigned int segno, i;
	int gc_type = sync ? FG_GC : BG_GC;
	int sec_freed = 0;
	int ret = -EINVAL;
	struct cp_control cpc;
	struct gc_inode_list gc_list = {
		.ilist = LIST_HEAD_INIT(gc_list.ilist),
		.iroot = RADIX_TREE_INIT(GFP_NOFS),
	};

	cpc.reason = __get_cp_reason(sbi);
gc_more:
	segno = NULL_SEGNO;

	if (unlikely(!(sbi->sb->s_flags & MS_ACTIVE)))
		goto stop;
	if (unlikely(f2fs_cp_error(sbi)))
		goto stop;

	if (gc_type == BG_GC && has_not_enough_free_secs(sbi, sec_freed)) {
		gc_type = FG_GC;
		if (__get_victim(sbi, &segno, gc_type) || prefree_segments(sbi))
			write_checkpoint(sbi, &cpc);
	}

	if (segno == NULL_SEGNO && !__get_victim(sbi, &segno, gc_type))
		goto stop;
	ret = 0;

	/* readahead multi ssa blocks those have contiguous address */
	if (sbi->segs_per_sec > 1)
		ra_meta_pages(sbi, GET_SUM_BLOCK(sbi, segno), sbi->segs_per_sec,
							META_SSA, true);

	for (i = 0; i < sbi->segs_per_sec; i++) {
		/*
		 * for FG_GC case, halt gcing left segments once failed one
		 * of segments in selected section to avoid long latency.
		 */
		if (!do_garbage_collect(sbi, segno + i, &gc_list, gc_type) &&
				gc_type == FG_GC)
			break;
	}

	if (i == sbi->segs_per_sec && gc_type == FG_GC)
		sec_freed++;

	if (gc_type == FG_GC)
		sbi->cur_victim_sec = NULL_SEGNO;

	if (!sync) {
		if (has_not_enough_free_secs(sbi, sec_freed))
			goto gc_more;

		if (gc_type == FG_GC)
			write_checkpoint(sbi, &cpc);
	}
stop:
	mutex_unlock(&sbi->gc_mutex);

	put_gc_inode(&gc_list);

	if (sync)
		ret = sec_freed ? 0 : -EAGAIN;
	return ret;
}
