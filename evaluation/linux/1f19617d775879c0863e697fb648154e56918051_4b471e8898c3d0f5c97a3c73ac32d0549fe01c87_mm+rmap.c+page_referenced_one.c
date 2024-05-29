static int page_referenced_one(struct page *page, struct vm_area_struct *vma,
			unsigned long address, void *arg)
{
	struct mm_struct *mm = vma->vm_mm;
	spinlock_t *ptl;
	int referenced = 0;
	struct page_referenced_arg *pra = arg;

	if (unlikely(PageTransHuge(page))) {
		pmd_t *pmd;

		/*
		 * rmap might return false positives; we must filter
		 * these out using page_check_address_pmd().
		 */
		pmd = page_check_address_pmd(page, mm, address, &ptl);
		if (!pmd)
			return SWAP_AGAIN;

		if (vma->vm_flags & VM_LOCKED) {
			spin_unlock(ptl);
			pra->vm_flags |= VM_LOCKED;
			return SWAP_FAIL; /* To break the loop */
		}

		if (pmdp_clear_flush_young_notify(vma, address, pmd))
			referenced++;
		spin_unlock(ptl);
	} else {
		pte_t *pte;

		/*
		 * rmap might return false positives; we must filter
		 * these out using page_check_address().
		 */
		pte = page_check_address(page, mm, address, &ptl, 0);
		if (!pte)
			return SWAP_AGAIN;

		if (vma->vm_flags & VM_LOCKED) {
			pte_unmap_unlock(pte, ptl);
			pra->vm_flags |= VM_LOCKED;
			return SWAP_FAIL; /* To break the loop */
		}

		if (ptep_clear_flush_young_notify(vma, address, pte)) {
			/*
			 * Don't treat a reference through a sequentially read
			 * mapping as such.  If the page has been used in
			 * another mapping, we will catch it; if this other
			 * mapping is already gone, the unmap path will have
			 * set PG_referenced or activated the page.
			 */
			if (likely(!(vma->vm_flags & VM_SEQ_READ)))
				referenced++;
		}
		pte_unmap_unlock(pte, ptl);
	}

	if (referenced)
		clear_page_idle(page);
	if (test_and_clear_page_young(page))
		referenced++;

	if (referenced) {
		pra->referenced++;
		pra->vm_flags |= vma->vm_flags;
	}

	pra->mapcount--;
	if (!pra->mapcount)
		return SWAP_SUCCESS; /* To break the loop */

	return SWAP_AGAIN;
}
