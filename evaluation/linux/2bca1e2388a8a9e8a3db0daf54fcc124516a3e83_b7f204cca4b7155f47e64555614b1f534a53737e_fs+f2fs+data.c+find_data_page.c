struct page *find_data_page(struct inode *inode, pgoff_t index, bool sync)
{
	struct address_space *mapping = inode->i_mapping;
	struct dnode_of_data dn;
	struct page *page;
	struct extent_info ei;
	int err;
	struct f2fs_io_info fio = {
		.type = DATA,
		.rw = sync ? READ_SYNC : READA,
	};

	/*
	 * If sync is false, it needs to check its block allocation.
	 * This is need and triggered by two flows:
	 *   gc and truncate_partial_data_page.
	 */
	if (!sync)
		goto search;

	page = find_get_page(mapping, index);
	if (page && PageUptodate(page))
		return page;
	f2fs_put_page(page, 0);
search:
	if (f2fs_lookup_extent_cache(inode, index, &ei)) {
		dn.data_blkaddr = ei.blk + index - ei.fofs;
		goto got_it;
	}

	set_new_dnode(&dn, inode, NULL, NULL, 0);
	err = get_dnode_of_data(&dn, index, LOOKUP_NODE);
	if (err)
		return ERR_PTR(err);
	f2fs_put_dnode(&dn);

	if (dn.data_blkaddr == NULL_ADDR)
		return ERR_PTR(-ENOENT);

	/* By fallocate(), there is no cached page, but with NEW_ADDR */
	if (unlikely(dn.data_blkaddr == NEW_ADDR))
		return ERR_PTR(-EINVAL);

got_it:
	page = grab_cache_page(mapping, index);
	if (!page)
		return ERR_PTR(-ENOMEM);

	if (PageUptodate(page)) {
		unlock_page(page);
		return page;
	}

	fio.blk_addr = dn.data_blkaddr;
	err = f2fs_submit_page_bio(F2FS_I_SB(inode), page, &fio);
	if (err)
		return ERR_PTR(err);

	if (sync) {
		wait_on_page_locked(page);
		if (unlikely(!PageUptodate(page))) {
			f2fs_put_page(page, 0);
			return ERR_PTR(-EIO);
		}
	}
	return page;
}
