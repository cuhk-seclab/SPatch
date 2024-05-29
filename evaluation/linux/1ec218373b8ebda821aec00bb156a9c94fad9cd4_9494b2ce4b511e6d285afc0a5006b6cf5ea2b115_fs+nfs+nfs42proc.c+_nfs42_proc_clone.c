static int _nfs42_proc_clone(struct rpc_message *msg, struct file *src_f,
			     struct file *dst_f, loff_t src_offset,
			     loff_t dst_offset, loff_t count)
{
	struct inode *src_inode = file_inode(src_f);
	struct inode *dst_inode = file_inode(dst_f);
	struct nfs_server *server = NFS_SERVER(dst_inode);
	struct nfs42_clone_args args = {
		.src_fh = NFS_FH(src_inode),
		.dst_fh = NFS_FH(dst_inode),
		.src_offset = src_offset,
		.dst_offset = dst_offset,
		.count = count,
		.dst_bitmask = server->cache_consistency_bitmask,
	};
	struct nfs42_clone_res res = {
		.server	= server,
	};
	int status;

	msg->rpc_argp = &args;
	msg->rpc_resp = &res;

	status = nfs42_set_rw_stateid(&args.src_stateid, src_f, FMODE_READ);
	if (status)
		return status;

	status = nfs42_set_rw_stateid(&args.dst_stateid, dst_f, FMODE_WRITE);
	if (status)
		return status;

	res.dst_fattr = nfs_alloc_fattr();
	if (!res.dst_fattr)
		return -ENOMEM;

	status = nfs4_call_sync(server->client, server, msg,
				&args.seq_args, &res.seq_res, 0);
	if (status == 0)
		status = nfs_post_op_update_inode(dst_inode, res.dst_fattr);

	kfree(res.dst_fattr);
	return status;
}
