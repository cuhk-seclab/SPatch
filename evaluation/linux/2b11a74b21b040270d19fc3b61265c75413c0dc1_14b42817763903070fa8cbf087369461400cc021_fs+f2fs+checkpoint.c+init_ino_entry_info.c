void init_ino_entry_info(struct f2fs_sb_info *sbi)
{
	int i;

	for (i = 0; i < MAX_INO_ENTRY; i++) {
		struct inode_management *im = &sbi->im[i];

		INIT_RADIX_TREE(&im->ino_root, GFP_ATOMIC);
		spin_lock_init(&im->ino_lock);
		INIT_LIST_HEAD(&im->ino_list);
		im->ino_num = 0;
	}

	/*
	 * considering 512 blocks in a segment 8+cp_payload blocks are
	 * needed for cp and log segment summaries. Remaining blocks are
	 * used to keep orphan entries with the limitation one reserved
	 * segment for cp pack we can have max 1020*(504-cp_payload)
	 * orphan entries
	 */
	sbi->max_orphans = (sbi->blocks_per_seg - F2FS_CP_PACKS -
			NR_CURSEG_TYPE - __cp_payload(sbi)) *
				F2FS_ORPHANS_PER_BLOCK;
}
