int nfs_readpage_async(struct nfs_open_context *ctx, struct inode *inode,
		       struct page *page)
{
	struct nfs_page	*new;
	unsigned int len;
	struct nfs_pageio_descriptor pgio;
	struct nfs_pgio_mirror *pgm;

	len = nfs_page_length(page);
	if (len == 0)
		return nfs_return_empty_page(page);
	new = nfs_create_request(ctx, page, NULL, 0, len);
	if (IS_ERR(new)) {
		unlock_page(page);
		return PTR_ERR(new);
	}
	if (len < PAGE_CACHE_SIZE)
		zero_user_segment(page, len, PAGE_CACHE_SIZE);

	nfs_pageio_init_read(&pgio, inode, false,
			     &nfs_async_read_completion_ops);
	if (!nfs_pageio_add_request(&pgio, new)) {
		nfs_list_remove_request(new);
		nfs_readpage_release(new);
	}
	nfs_pageio_complete(&pgio);

	/* It doesn't make sense to do mirrored reads! */
	WARN_ON_ONCE(pgio.pg_mirror_count != 1);

	pgm = &pgio.pg_mirrors[0];
	NFS_I(inode)->read_io += pgm->pg_bytes_written;

	return pgio.pg_error < 0 ? pgio.pg_error : 0;
}
