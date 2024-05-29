static int nfs_page_async_flush(struct nfs_pageio_descriptor *pgio,
				struct page *page, bool nonblock)
{
	struct nfs_page *req;
	int ret = 0;

	req = nfs_lock_and_join_requests(page, nonblock);
	if (!req)
		goto out;
	ret = PTR_ERR(req);
	if (IS_ERR(req))
		goto out;

	nfs_set_page_writeback(page);
	WARN_ON_ONCE(test_bit(PG_CLEAN, &req->wb_flags));

	ret = 0;
	if (!nfs_pageio_add_request(pgio, req)) {
		ret = pgio->pg_error;
		/*
		 * Remove the problematic req upon fatal errors,
		 * while other dirty pages can still be around
		 * until they get flushed.
		 */
		if (nfs_error_is_fatal(ret)) {
			nfs_context_set_write_error(req->wb_context, ret);
			nfs_write_error_remove_page(req);
		} else {
			nfs_redirty_request(req);
			ret = -EAGAIN;
		}
	} else
		nfs_add_stats(page_file_mapping(page)->host,
				NFSIOS_WRITEPAGES, 1);
out:
	return ret;
}
