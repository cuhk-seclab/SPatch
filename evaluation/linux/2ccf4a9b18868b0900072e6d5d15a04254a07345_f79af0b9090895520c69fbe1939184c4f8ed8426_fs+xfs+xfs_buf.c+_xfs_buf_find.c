xfs_buf_t *
_xfs_buf_find(
	struct xfs_buftarg	*btp,
	struct xfs_buf_map	*map,
	int			nmaps,
	xfs_buf_flags_t		flags,
	xfs_buf_t		*new_bp)
{
	struct xfs_perag	*pag;
	struct rb_node		**rbp;
	struct rb_node		*parent;
	xfs_buf_t		*bp;
	xfs_daddr_t		blkno = map[0].bm_bn;
	xfs_daddr_t		eofs;
	int			numblks = 0;
	int			i;

	for (i = 0; i < nmaps; i++)
		numblks += map[i].bm_len;

	/* Check for IOs smaller than the sector size / not sector aligned */
	ASSERT(!(BBTOB(numblks) < btp->bt_meta_sectorsize));
	ASSERT(!(BBTOB(blkno) & (xfs_off_t)btp->bt_meta_sectormask));

	/*
	 * Corrupted block numbers can get through to here, unfortunately, so we
	 * have to check that the buffer falls within the filesystem bounds.
	 */
	eofs = XFS_FSB_TO_BB(btp->bt_mount, btp->bt_mount->m_sb.sb_dblocks);
	if (blkno < 0 || blkno >= eofs) {
		/*
		 * XXX (dgc): we should really be returning -EFSCORRUPTED here,
		 * but none of the higher level infrastructure supports
		 * returning a specific error on buffer lookup failures.
		 */
		xfs_alert(btp->bt_mount,
			  "%s: Block out of range: block 0x%llx, EOFS 0x%llx ",
			  __func__, blkno, eofs);
		WARN_ON(1);
		return NULL;
	}

	/* get tree root */
	pag = xfs_perag_get(btp->bt_mount,
				xfs_daddr_to_agno(btp->bt_mount, blkno));

	/* walk tree */
	spin_lock(&pag->pag_buf_lock);
	rbp = &pag->pag_buf_tree.rb_node;
	parent = NULL;
	bp = NULL;
	while (*rbp) {
		parent = *rbp;
		bp = rb_entry(parent, struct xfs_buf, b_rbnode);

		if (blkno < bp->b_bn)
			rbp = &(*rbp)->rb_left;
		else if (blkno > bp->b_bn)
			rbp = &(*rbp)->rb_right;
		else {
			/*
			 * found a block number match. If the range doesn't
			 * match, the only way this is allowed is if the buffer
			 * in the cache is stale and the transaction that made
			 * it stale has not yet committed. i.e. we are
			 * reallocating a busy extent. Skip this buffer and
			 * continue searching to the right for an exact match.
			 */
			if (bp->b_length != numblks) {
				ASSERT(bp->b_flags & XBF_STALE);
				rbp = &(*rbp)->rb_right;
				continue;
			}
			atomic_inc(&bp->b_hold);
			goto found;
		}
	}

	/* No match found */
	if (new_bp) {
		rb_link_node(&new_bp->b_rbnode, parent, rbp);
		rb_insert_color(&new_bp->b_rbnode, &pag->pag_buf_tree);
		/* the buffer keeps the perag reference until it is freed */
		new_bp->b_pag = pag;
		spin_unlock(&pag->pag_buf_lock);
	} else {
		XFS_STATS_INC(xb_miss_locked);
		spin_unlock(&pag->pag_buf_lock);
		xfs_perag_put(pag);
	}
	return new_bp;

found:
	spin_unlock(&pag->pag_buf_lock);
	xfs_perag_put(pag);

	if (!xfs_buf_trylock(bp)) {
		if (flags & XBF_TRYLOCK) {
			xfs_buf_rele(bp);
			XFS_STATS_INC(xb_busy_locked);
			return NULL;
		}
		xfs_buf_lock(bp);
		XFS_STATS_INC(xb_get_locked_waited);
	}

	/*
	 * if the buffer is stale, clear all the external state associated with
	 * it. We need to keep flags such as how we allocated the buffer memory
	 * intact here.
	 */
	if (bp->b_flags & XBF_STALE) {
		ASSERT((bp->b_flags & _XBF_DELWRI_Q) == 0);
		ASSERT(bp->b_iodone == NULL);
		bp->b_flags &= _XBF_KMEM | _XBF_PAGES;
		bp->b_ops = NULL;
	}

	trace_xfs_buf_find(bp, flags, _RET_IP_);
	XFS_STATS_INC(xb_get_locked);
	return bp;
}
