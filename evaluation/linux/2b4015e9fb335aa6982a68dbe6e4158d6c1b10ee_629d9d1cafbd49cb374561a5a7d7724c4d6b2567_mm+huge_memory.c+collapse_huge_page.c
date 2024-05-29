static void collapse_huge_page(struct mm_struct *mm,
				   unsigned long address,
				   struct page **hpage,
				   struct vm_area_struct *vma,
				   int node)
{
	pmd_t *pmd, _pmd;
	pte_t *pte;
	pgtable_t pgtable;
	struct page *new_page;
	spinlock_t *pmd_ptl, *pte_ptl;
	int isolated = 0, result = 0;
	unsigned long hstart, hend;
	struct mem_cgroup *memcg;
	unsigned long mmun_start;	/* For mmu_notifiers */
	unsigned long mmun_end;		/* For mmu_notifiers */
	gfp_t gfp;

	VM_BUG_ON(address & ~HPAGE_PMD_MASK);

	/* Only allocate from the target node */
	gfp = alloc_hugepage_gfpmask(khugepaged_defrag(), __GFP_OTHER_NODE) |
		__GFP_THISNODE;

	/* release the mmap_sem read lock. */
	new_page = khugepaged_alloc_page(hpage, gfp, mm, address, node);
	if (!new_page) {
		result = SCAN_ALLOC_HUGE_PAGE_FAIL;
		goto out_nolock;
	}

	if (unlikely(mem_cgroup_try_charge(new_page, mm, gfp, &memcg, true))) {
		result = SCAN_CGROUP_CHARGE_FAIL;
		goto out_nolock;
	}

	/*
	 * Prevent all access to pagetables with the exception of
	 * gup_fast later hanlded by the ptep_clear_flush and the VM
	 * handled by the anon_vma lock + PG_lock.
	 */
	down_write(&mm->mmap_sem);
	if (unlikely(khugepaged_test_exit(mm))) {
		result = SCAN_ANY_PROCESS;
		goto out;
	}

	vma = find_vma(mm, address);
	if (!vma) {
		result = SCAN_VMA_NULL;
		goto out;
	}
	hstart = (vma->vm_start + ~HPAGE_PMD_MASK) & HPAGE_PMD_MASK;
	hend = vma->vm_end & HPAGE_PMD_MASK;
	if (address < hstart || address + HPAGE_PMD_SIZE > hend) {
		result = SCAN_ADDRESS_RANGE;
		goto out;
	}
	if (!hugepage_vma_check(vma)) {
		result = SCAN_VMA_CHECK;
		goto out;
	}
	pmd = mm_find_pmd(mm, address);
	if (!pmd) {
		result = SCAN_PMD_NULL;
		goto out;
	}

	anon_vma_lock_write(vma->anon_vma);

	pte = pte_offset_map(pmd, address);
	pte_ptl = pte_lockptr(mm, pmd);

	mmun_start = address;
	mmun_end   = address + HPAGE_PMD_SIZE;
	mmu_notifier_invalidate_range_start(mm, mmun_start, mmun_end);
	pmd_ptl = pmd_lock(mm, pmd); /* probably unnecessary */
	/*
	 * After this gup_fast can't run anymore. This also removes
	 * any huge TLB entry from the CPU so we won't allow
	 * huge and small TLB entries for the same virtual address
	 * to avoid the risk of CPU bugs in that area.
	 */
	_pmd = pmdp_collapse_flush(vma, address, pmd);
	spin_unlock(pmd_ptl);
	mmu_notifier_invalidate_range_end(mm, mmun_start, mmun_end);

	spin_lock(pte_ptl);
	isolated = __collapse_huge_page_isolate(vma, address, pte);
	spin_unlock(pte_ptl);

	if (unlikely(!isolated)) {
		pte_unmap(pte);
		spin_lock(pmd_ptl);
		BUG_ON(!pmd_none(*pmd));
		/*
		 * We can only use set_pmd_at when establishing
		 * hugepmds and never for establishing regular pmds that
		 * points to regular pagetables. Use pmd_populate for that
		 */
		pmd_populate(mm, pmd, pmd_pgtable(_pmd));
		spin_unlock(pmd_ptl);
		anon_vma_unlock_write(vma->anon_vma);
		result = SCAN_FAIL;
		goto out;
	}

	/*
	 * All pages are isolated and locked so anon_vma rmap
	 * can't run anymore.
	 */
	anon_vma_unlock_write(vma->anon_vma);

	__collapse_huge_page_copy(pte, new_page, vma, address, pte_ptl);
	pte_unmap(pte);
	__SetPageUptodate(new_page);
	pgtable = pmd_pgtable(_pmd);

	_pmd = mk_huge_pmd(new_page, vma->vm_page_prot);
	_pmd = maybe_pmd_mkwrite(pmd_mkdirty(_pmd), vma);

	/*
	 * spin_lock() below is not the equivalent of smp_wmb(), so
	 * this is needed to avoid the copy_huge_page writes to become
	 * visible after the set_pmd_at() write.
	 */
	smp_wmb();

	spin_lock(pmd_ptl);
	BUG_ON(!pmd_none(*pmd));
	page_add_new_anon_rmap(new_page, vma, address, true);
	mem_cgroup_commit_charge(new_page, memcg, false, true);
	lru_cache_add_active_or_unevictable(new_page, vma);
	pgtable_trans_huge_deposit(mm, pmd, pgtable);
	set_pmd_at(mm, address, pmd, _pmd);
	update_mmu_cache_pmd(vma, address, pmd);
	spin_unlock(pmd_ptl);

	*hpage = NULL;

	khugepaged_pages_collapsed++;
	result = SCAN_SUCCEED;
out_up_write:
	up_write(&mm->mmap_sem);
	trace_mm_collapse_huge_page(mm, isolated, result);
	return;

out_nolock:
	trace_mm_collapse_huge_page(mm, isolated, result);
	return;
out:
	mem_cgroup_cancel_charge(new_page, memcg, true);
	goto out_up_write;
}
