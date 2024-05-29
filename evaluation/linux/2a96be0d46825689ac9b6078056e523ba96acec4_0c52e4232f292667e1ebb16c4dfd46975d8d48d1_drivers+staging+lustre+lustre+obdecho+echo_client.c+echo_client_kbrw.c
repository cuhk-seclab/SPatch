static int echo_client_kbrw(struct echo_device *ed, int rw, struct obdo *oa,
			    struct echo_object *eco, u64 offset,
			    u64 count, int async,
			    struct obd_trans_info *oti)
{
	struct lov_stripe_md   *lsm = eco->eo_lsm;
	u32	       npages;
	struct brw_page	*pga;
	struct brw_page	*pgp;
	struct page	    **pages;
	u64		 off;
	int		     i;
	int		     rc;
	int		     verify;
	gfp_t		     gfp_mask;
	int		     brw_flags = 0;

	verify = (ostid_id(&oa->o_oi) != ECHO_PERSISTENT_OBJID &&
		  (oa->o_valid & OBD_MD_FLFLAGS) != 0 &&
		  (oa->o_flags & OBD_FL_DEBUG_CHECK) != 0);

	gfp_mask = ((ostid_id(&oa->o_oi) & 2) == 0) ? GFP_IOFS : GFP_HIGHUSER;

	LASSERT(rw == OBD_BRW_WRITE || rw == OBD_BRW_READ);
	LASSERT(lsm != NULL);
	LASSERT(ostid_id(&lsm->lsm_oi) == ostid_id(&oa->o_oi));

	if (count <= 0 ||
	    (count & (~CFS_PAGE_MASK)) != 0)
		return -EINVAL;

	/* XXX think again with misaligned I/O */
	npages = count >> PAGE_CACHE_SHIFT;

	if (rw == OBD_BRW_WRITE)
		brw_flags = OBD_BRW_ASYNC;

	pga = kcalloc(npages, sizeof(*pga), GFP_NOFS);
	if (pga == NULL)
		return -ENOMEM;

	pages = kcalloc(npages, sizeof(*pages), GFP_NOFS);
	if (pages == NULL) {
		kfree(pga);
		return -ENOMEM;
	}

	for (i = 0, pgp = pga, off = offset;
	     i < npages;
	     i++, pgp++, off += PAGE_CACHE_SIZE) {

		LASSERT(pgp->pg == NULL);      /* for cleanup */

		rc = -ENOMEM;
		OBD_PAGE_ALLOC(pgp->pg, gfp_mask);
		if (pgp->pg == NULL)
			goto out;

		pages[i] = pgp->pg;
		pgp->count = PAGE_CACHE_SIZE;
		pgp->off = off;
		pgp->flag = brw_flags;

		if (verify)
			echo_client_page_debug_setup(lsm, pgp->pg, rw,
						     ostid_id(&oa->o_oi), off,
						     pgp->count);
	}

	/* brw mode can only be used at client */
	LASSERT(ed->ed_next != NULL);
	rc = cl_echo_object_brw(eco, rw, offset, pages, npages, async);

 out:
	if (rc != 0 || rw != OBD_BRW_READ)
		verify = 0;

	for (i = 0, pgp = pga; i < npages; i++, pgp++) {
		if (pgp->pg == NULL)
			continue;

		if (verify) {
			int vrc;

			vrc = echo_client_page_debug_check(lsm, pgp->pg,
							   ostid_id(&oa->o_oi),
							   pgp->off, pgp->count);
			if (vrc != 0 && rc == 0)
				rc = vrc;
		}
		OBD_PAGE_FREE(pgp->pg);
	}
	kfree(pga);
	kfree(pages);
	return rc;
}
