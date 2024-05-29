static struct pnfs_layout_segment *
send_layoutget(struct pnfs_layout_hdr *lo,
	   struct nfs_open_context *ctx,
	   struct pnfs_layout_range *range,
	   gfp_t gfp_flags)
{
	struct inode *ino = lo->plh_inode;
	struct nfs_server *server = NFS_SERVER(ino);
	struct nfs4_layoutget *lgp;
	struct pnfs_layout_segment *lseg;
	loff_t i_size;

	dprintk("--> %s\n", __func__);

	/*
	 * Synchronously retrieve layout information from server and
	 * store in lseg. If we race with a concurrent seqid morphing
	 * op, then re-send the LAYOUTGET.
	 */
	do {
		lgp = kzalloc(sizeof(*lgp), gfp_flags);
		if (lgp == NULL)
			return NULL;

		i_size = i_size_read(ino);

		lgp->args.minlength = PAGE_CACHE_SIZE;
		if (lgp->args.minlength > range->length)
			lgp->args.minlength = range->length;
		if (range->iomode == IOMODE_READ) {
			if (range->offset >= i_size)
				lgp->args.minlength = 0;
			else if (i_size - range->offset < lgp->args.minlength)
				lgp->args.minlength = i_size - range->offset;
		}
		lgp->args.maxcount = PNFS_LAYOUT_MAXSIZE;
		lgp->args.range = *range;
		lgp->args.type = server->pnfs_curr_ld->id;
		lgp->args.inode = ino;
		lgp->args.ctx = get_nfs_open_context(ctx);
		lgp->gfp_flags = gfp_flags;
		lgp->cred = lo->plh_lc_cred;

		lseg = nfs4_proc_layoutget(lgp, gfp_flags);
	} while (lseg == ERR_PTR(-EAGAIN));

	if (IS_ERR(lseg) && !nfs_error_is_fatal(PTR_ERR(lseg)))
		lseg = NULL;
	else
		pnfs_layout_clear_fail_bit(lo,
				pnfs_iomode_to_fail_bit(range->iomode));

	return lseg;
}
