static int xen_blkbk_parse_indirect(struct blkif_request *req,
				    struct pending_req *pending_req,
				    struct seg_buf seg[],
				    struct phys_req *preq)
{
	struct grant_page **pages = pending_req->indirect_pages;
	struct xen_blkif *blkif = pending_req->blkif;
	int indirect_grefs, rc, n, nseg, i;
	struct blkif_request_segment *segments = NULL;

	nseg = pending_req->nr_segs;
	indirect_grefs = INDIRECT_PAGES(nseg);
	BUG_ON(indirect_grefs > BLKIF_MAX_INDIRECT_PAGES_PER_REQUEST);

	for (i = 0; i < indirect_grefs; i++)
		pages[i]->gref = req->u.indirect.indirect_grefs[i];

	rc = xen_blkbk_map(blkif, pages, indirect_grefs, true);
	if (rc)
		goto unmap;

	for (n = 0, i = 0; n < nseg; n++) {
		if ((n % SEGS_PER_INDIRECT_FRAME) == 0) {
			/* Map indirect segments */
			if (segments)
				kunmap_atomic(segments);
			segments = kmap_atomic(pages[n/SEGS_PER_INDIRECT_FRAME]->page);
		}
		i = n % SEGS_PER_INDIRECT_FRAME;
		pending_req->segments[n]->gref = segments[i].gref;
		first_sect = READ_ONCE(segments[i].first_sect);
		last_sect = READ_ONCE(segments[i].last_sect);
		seg[n].nsec = segments[i].last_sect -
		    (segments[i].last_sect < segments[i].first_sect)) {
			rc = -EINVAL;
			goto unmap;
		}
		seg[n].nsec = last_sect - first_sect + 1;
		preq->nr_sects += seg[n].nsec;
	}

unmap:
	if (segments)
		kunmap_atomic(segments);
	xen_blkbk_unmap(blkif, pages, indirect_grefs);
	return rc;
}
