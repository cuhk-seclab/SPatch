int
xfs_buf_item_init(
	struct xfs_buf	*bp,
	struct xfs_mount *mp)
{
	struct xfs_log_item	*lip = bp->b_fspriv;
	struct xfs_buf_log_item	*bip;
	int			chunks;
	int			map_size;
	int			error;
	int			i;

	/*
	 * Check to see if there is already a buf log item for
	 * this buffer.  If there is, it is guaranteed to be
	 * the first.  If we do already have one, there is
	 * nothing to do here so return.
	 */
	ASSERT(bp->b_target->bt_mount == mp);
	if (lip != NULL && lip->li_type == XFS_LI_BUF)
		return 0;

	bip = kmem_zone_zalloc(xfs_buf_item_zone, KM_SLEEP);
	xfs_log_item_init(mp, &bip->bli_item, XFS_LI_BUF, &xfs_buf_item_ops);
	bip->bli_buf = bp;

	/*
	 * chunks is the number of XFS_BLF_CHUNK size pieces the buffer
	 * can be divided into. Make sure not to truncate any pieces.
	 * map_size is the size of the bitmap needed to describe the
	 * chunks of the buffer.
	 *
	 * Discontiguous buffer support follows the layout of the underlying
	 * buffer. This makes the implementation as simple as possible.
	 */
	error = xfs_buf_item_get_format(bip, bp->b_map_count);
	ASSERT(error == 0);
	if (error) {	/* to stop gcc throwing set-but-unused warnings */
		kmem_zone_free(xfs_buf_item_zone, bip);
		return error;
	}


	for (i = 0; i < bip->bli_format_count; i++) {
		chunks = DIV_ROUND_UP(BBTOB(bp->b_maps[i].bm_len),
				      XFS_BLF_CHUNK);
		map_size = DIV_ROUND_UP(chunks, NBWORD);

		bip->bli_formats[i].blf_type = XFS_LI_BUF;
		bip->bli_formats[i].blf_blkno = bp->b_maps[i].bm_bn;
		bip->bli_formats[i].blf_len = bp->b_maps[i].bm_len;
		bip->bli_formats[i].blf_map_size = map_size;
	}

	/*
	 * Put the buf item into the list of items attached to the
	 * buffer at the front.
	 */
	if (bp->b_fspriv)
		bip->bli_item.li_bio_list = bp->b_fspriv;
	bp->b_fspriv = bip;
	xfs_buf_hold(bp);
	return 0;
}
