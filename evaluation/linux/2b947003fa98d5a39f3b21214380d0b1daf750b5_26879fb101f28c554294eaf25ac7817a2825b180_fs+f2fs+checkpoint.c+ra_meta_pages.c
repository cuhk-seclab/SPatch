int ra_meta_pages(struct f2fs_sb_info *sbi, block_t start, int nrpages,
							int type, bool sync)
{
	block_t prev_blk_addr = 0;
	struct page *page;
	block_t blkno = start;
	struct f2fs_io_info fio = {
		.sbi = sbi,
		.type = META,
		.rw = sync ? (READ_SYNC | REQ_META | REQ_PRIO) : READA,
		.encrypted_page = NULL,
	};

	if (unlikely(type == META_POR))
		fio.rw &= ~REQ_META;

	for (; nrpages-- > 0; blkno++) {

		if (!is_valid_blkaddr(sbi, blkno, type))
			goto out;

		switch (type) {
		case META_NAT:
			if (unlikely(blkno >=
					NAT_BLOCK_OFFSET(NM_I(sbi)->max_nid)))
				blkno = 0;
			/* get nat block addr */
			fio.blk_addr = current_nat_addr(sbi,
					blkno * NAT_ENTRY_PER_BLOCK);
			break;
		case META_SIT:
			/* get sit block addr */
			fio.blk_addr = current_sit_addr(sbi,
					blkno * SIT_ENTRY_PER_BLOCK);
			if (blkno != start && prev_blk_addr + 1 != fio.blk_addr)
				goto out;
			prev_blk_addr = fio.blk_addr;
			break;
		case META_SSA:
		case META_CP:
		case META_POR:
			fio.blk_addr = blkno;
			break;
		default:
			BUG();
		}

		page = grab_cache_page(META_MAPPING(sbi), fio.blk_addr);
		if (!page)
			continue;
		if (PageUptodate(page)) {
			f2fs_put_page(page, 1);
			continue;
		}

		fio.page = page;
		f2fs_submit_page_mbio(&fio);
		f2fs_put_page(page, 0);
	}
out:
	f2fs_submit_merged_bio(sbi, META, READ);
	return blkno - start;
}
