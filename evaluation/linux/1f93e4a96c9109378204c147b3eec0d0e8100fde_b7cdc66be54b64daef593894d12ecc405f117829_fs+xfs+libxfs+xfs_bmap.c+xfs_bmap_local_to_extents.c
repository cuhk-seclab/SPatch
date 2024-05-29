STATIC int				/* error */
xfs_bmap_local_to_extents(
	xfs_trans_t	*tp,		/* transaction pointer */
	xfs_inode_t	*ip,		/* incore inode pointer */
	xfs_fsblock_t	*firstblock,	/* first block allocated in xaction */
	xfs_extlen_t	total,		/* total blocks needed by transaction */
	int		*logflagsp,	/* inode logging flags */
	int		whichfork,
	void		(*init_fn)(struct xfs_trans *tp,
				   struct xfs_buf *bp,
				   struct xfs_inode *ip,
				   struct xfs_ifork *ifp))
{
	int		error = 0;
	int		flags;		/* logging flags returned */
	xfs_ifork_t	*ifp;		/* inode fork pointer */
	xfs_alloc_arg_t	args;		/* allocation arguments */
	xfs_buf_t	*bp;		/* buffer for extent block */
	xfs_bmbt_rec_host_t *ep;	/* extent record pointer */

	/*
	 * We don't want to deal with the case of keeping inode data inline yet.
	 * So sending the data fork of a regular inode is invalid.
	 */
	ASSERT(!(S_ISREG(ip->i_d.di_mode) && whichfork == XFS_DATA_FORK));
	ifp = XFS_IFORK_PTR(ip, whichfork);
	ASSERT(XFS_IFORK_FORMAT(ip, whichfork) == XFS_DINODE_FMT_LOCAL);

	if (!ifp->if_bytes) {
		xfs_bmap_local_to_extents_empty(ip, whichfork);
		flags = XFS_ILOG_CORE;
		goto done;
	}

	flags = 0;
	error = 0;
	ASSERT((ifp->if_flags & (XFS_IFINLINE|XFS_IFEXTENTS|XFS_IFEXTIREC)) ==
								XFS_IFINLINE);
	memset(&args, 0, sizeof(args));
	args.tp = tp;
	args.mp = ip->i_mount;
	args.firstblock = *firstblock;
	/*
	 * Allocate a block.  We know we need only one, since the
	 * file currently fits in an inode.
	 */
	if (*firstblock == NULLFSBLOCK) {
		args.fsbno = XFS_INO_TO_FSB(args.mp, ip->i_ino);
		args.type = XFS_ALLOCTYPE_START_BNO;
	} else {
		args.fsbno = *firstblock;
		args.type = XFS_ALLOCTYPE_NEAR_BNO;
	}
	args.total = total;
	args.minlen = args.maxlen = args.prod = 1;
	error = xfs_alloc_vextent(&args);
	if (error)
		goto done;

	/* Can't fail, the space was reserved. */
	ASSERT(args.fsbno != NULLFSBLOCK);
	ASSERT(args.len == 1);
	*firstblock = args.fsbno;
	bp = xfs_btree_get_bufl(args.mp, tp, args.fsbno, 0);

	/*
	 * Initialize the block, copy the data and log the remote buffer.
	 *
	 * The callout is responsible for logging because the remote format
	 * might differ from the local format and thus we don't know how much to
	 * log here. Note that init_fn must also set the buffer log item type
	 * correctly.
	 */
	init_fn(tp, bp, ip, ifp);

	/* account for the change in fork size */
	xfs_idata_realloc(ip, -ifp->if_bytes, whichfork);
	xfs_bmap_local_to_extents_empty(ip, whichfork);
	flags |= XFS_ILOG_CORE;

	xfs_iext_add(ifp, 0, 1);
	ep = xfs_iext_get_ext(ifp, 0);
	xfs_bmbt_set_allf(ep, 0, args.fsbno, 1, XFS_EXT_NORM);
	trace_xfs_bmap_post_update(ip, 0,
			whichfork == XFS_ATTR_FORK ? BMAP_ATTRFORK : 0,
			_THIS_IP_);
	XFS_IFORK_NEXT_SET(ip, whichfork, 1);
	ip->i_d.di_nblocks = 1;
	xfs_trans_mod_dquot_byino(tp, ip,
		XFS_TRANS_DQ_BCOUNT, 1L);
	flags |= xfs_ilog_fext(whichfork);

done:
	*logflagsp = flags;
	return error;
}
