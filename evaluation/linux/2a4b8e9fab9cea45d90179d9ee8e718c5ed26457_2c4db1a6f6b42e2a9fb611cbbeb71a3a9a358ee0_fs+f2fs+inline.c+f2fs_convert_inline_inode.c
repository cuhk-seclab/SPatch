int f2fs_convert_inline_inode(struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct dnode_of_data dn;
	struct page *ipage, *page;
	int err = 0;

	if (!f2fs_has_inline_data(inode))
		return 0;

	page = grab_cache_page(inode->i_mapping, 0);
	if (!page)
		return -ENOMEM;

	f2fs_lock_op(sbi);

	ipage = get_node_page(sbi, inode->i_ino);
	if (IS_ERR(ipage)) {
		err = PTR_ERR(ipage);
		goto out;
	}

	set_new_dnode(&dn, inode, ipage, ipage, 0);

	if (f2fs_has_inline_data(inode))
		err = f2fs_convert_inline_page(&dn, page);

	f2fs_put_dnode(&dn);
out:
	f2fs_unlock_op(sbi);

	f2fs_put_page(page, 1);

	f2fs_balance_fs(sbi, dn.node_changed);

	return err;
}
