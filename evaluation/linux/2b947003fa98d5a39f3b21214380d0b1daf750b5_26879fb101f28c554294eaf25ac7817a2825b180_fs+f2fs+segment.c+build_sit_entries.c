static void build_sit_entries(struct f2fs_sb_info *sbi)
{
	struct sit_info *sit_i = SIT_I(sbi);
	struct curseg_info *curseg = CURSEG_I(sbi, CURSEG_COLD_DATA);
	struct f2fs_summary_block *sum = curseg->sum_blk;
	int sit_blk_cnt = SIT_BLK_CNT(sbi);
	unsigned int i, start, end;
	unsigned int readed, start_blk = 0;
	int nrpages = MAX_BIO_BLOCKS(sbi);

	do {
		readed = ra_meta_pages(sbi, start_blk, nrpages, META_SIT);

		start = start_blk * sit_i->sents_per_block;
		end = (start_blk + readed) * sit_i->sents_per_block;

		for (; start < end && start < MAIN_SEGS(sbi); start++) {
			struct seg_entry *se = &sit_i->sentries[start];
			struct f2fs_sit_block *sit_blk;
			struct f2fs_sit_entry sit;
			struct page *page;

			mutex_lock(&curseg->curseg_mutex);
			for (i = 0; i < sits_in_cursum(sum); i++) {
				if (le32_to_cpu(segno_in_journal(sum, i))
								== start) {
					sit = sit_in_journal(sum, i);
					mutex_unlock(&curseg->curseg_mutex);
					goto got_it;
				}
			}
			mutex_unlock(&curseg->curseg_mutex);

			page = get_current_sit_page(sbi, start);
			sit_blk = (struct f2fs_sit_block *)page_address(page);
			sit = sit_blk->entries[SIT_ENTRY_OFFSET(sit_i, start)];
			f2fs_put_page(page, 1);
got_it:
			check_block_count(sbi, start, &sit);
			seg_info_from_raw_sit(se, &sit);

			/* build discard map only one time */
			memcpy(se->discard_map, se->cur_valid_map, SIT_VBLOCK_MAP_SIZE);
			sbi->discard_blks += sbi->blocks_per_seg - se->valid_blocks;

			if (sbi->segs_per_sec > 1) {
				struct sec_entry *e = get_sec_entry(sbi, start);
				e->valid_blocks += se->valid_blocks;
			}
		}
		start_blk += readed;
	} while (start_blk < sit_blk_cnt);
}
