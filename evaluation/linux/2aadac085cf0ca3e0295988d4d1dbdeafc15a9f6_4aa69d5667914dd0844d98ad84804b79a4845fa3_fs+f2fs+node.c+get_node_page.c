struct page *get_node_page(struct f2fs_sb_info *sbi, pgoff_t nid)
{
	struct page *page;
	int err;

	if (!nid)
		return ERR_PTR(-ENOENT);
	f2fs_bug_on(sbi, check_nid_range(sbi, nid));
repeat:
	page = grab_cache_page(NODE_MAPPING(sbi), nid);
	if (!page)
		return ERR_PTR(-ENOMEM);

	err = read_node_page(page, READ_SYNC);
	if (err < 0) {
		f2fs_put_page(page, 1);
		return ERR_PTR(err);
	} else if (err == LOCKED_PAGE) {
		goto page_hit;
	}

	lock_page(page);

	if (unlikely(!PageUptodate(page))) {
		f2fs_put_page(page, 1);
		return ERR_PTR(-EIO);
	}
	if (unlikely(page->mapping != NODE_MAPPING(sbi))) {
		f2fs_put_page(page, 1);
		goto repeat;
	}
page_hit:
	f2fs_bug_on(sbi, nid != nid_of_node(page));
	return page;
}
