int commit_inmem_pages(struct inode *inode, bool abort)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct inmem_pages *cur, *tmp;
	bool submit_bio = false;
	struct f2fs_io_info fio = {
		.sbi = sbi,
		.type = DATA,
		.rw = WRITE_SYNC | REQ_PRIO,
		.encrypted_page = NULL,
	};
	int err = 0;

	/*
	 * The abort is true only when f2fs_evict_inode is called.
	 * Basically, the f2fs_evict_inode doesn't produce any data writes, so
	 * that we don't need to call f2fs_balance_fs.
	 * Otherwise, f2fs_gc in f2fs_balance_fs can wait forever until this
	 * inode becomes free by iget_locked in f2fs_iget.
	 */
	if (!abort) {
		f2fs_balance_fs(sbi, true);
		f2fs_lock_op(sbi);
	}

	mutex_lock(&fi->inmem_lock);
	list_for_each_entry_safe(cur, tmp, &fi->inmem_pages, list) {
		lock_page(cur->page);
		if (!abort) {
			if (cur->page->mapping == inode->i_mapping) {
				set_page_dirty(cur->page);
				f2fs_wait_on_page_writeback(cur->page, DATA);
				if (clear_page_dirty_for_io(cur->page))
					inode_dec_dirty_pages(inode);
				trace_f2fs_commit_inmem_page(cur->page, INMEM);
				fio.page = cur->page;
				err = do_write_data_page(&fio);
				if (err) {
					unlock_page(cur->page);
					break;
				}
				clear_cold_data(cur->page);
				submit_bio = true;
			}
		} else {
			ClearPageUptodate(cur->page);
			trace_f2fs_commit_inmem_page(cur->page, INMEM_DROP);
		}
		set_page_private(cur->page, 0);
		ClearPagePrivate(cur->page);
		f2fs_put_page(cur->page, 1);

		list_del(&cur->list);
		kmem_cache_free(inmem_entry_slab, cur);
		dec_page_count(F2FS_I_SB(inode), F2FS_INMEM_PAGES);
	}
	mutex_unlock(&fi->inmem_lock);

	if (!abort) {
		f2fs_unlock_op(sbi);
		if (submit_bio)
			f2fs_submit_merged_bio(sbi, DATA, WRITE);
	}
	return err;
}
