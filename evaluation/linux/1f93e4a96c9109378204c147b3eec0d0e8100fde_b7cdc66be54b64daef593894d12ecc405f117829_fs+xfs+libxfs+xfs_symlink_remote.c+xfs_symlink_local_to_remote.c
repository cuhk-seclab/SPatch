void
xfs_symlink_local_to_remote(
	struct xfs_trans	*tp,
	struct xfs_buf		*bp,
	struct xfs_inode	*ip,
	struct xfs_ifork	*ifp)
{
	struct xfs_mount	*mp = ip->i_mount;
	char			*buf;

	xfs_trans_buf_set_type(tp, bp, XFS_BLFT_SYMLINK_BUF);

	if (!xfs_sb_version_hascrc(&mp->m_sb)) {
		bp->b_ops = NULL;
		memcpy(bp->b_addr, ifp->if_u1.if_data, ifp->if_bytes);
		xfs_trans_log_buf(tp, bp, 0, ifp->if_bytes - 1);
		return;
	}

	/*
	 * As this symlink fits in an inode literal area, it must also fit in
	 * the smallest buffer the filesystem supports.
	 */
	ASSERT(BBTOB(bp->b_length) >=
			ifp->if_bytes + sizeof(struct xfs_dsymlink_hdr));

	bp->b_ops = &xfs_symlink_buf_ops;

	buf = bp->b_addr;
	buf += xfs_symlink_hdr_set(mp, ip->i_ino, 0, ifp->if_bytes, bp);
	memcpy(buf, ifp->if_u1.if_data, ifp->if_bytes);
	xfs_trans_log_buf(tp, bp, 0, sizeof(struct xfs_dsymlink_hdr) +
					ifp->if_bytes - 1);
}
