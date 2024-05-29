unsigned long move_page_tables(struct vm_area_struct *vma,
		unsigned long old_addr, struct vm_area_struct *new_vma,
		unsigned long new_addr, unsigned long len,
		bool need_rmap_locks)
{
	unsigned long extent, next, old_end;
	pmd_t *old_pmd, *new_pmd;
	bool need_flush = false;
	unsigned long mmun_start;	/* For mmu_notifiers */
	unsigned long mmun_end;		/* For mmu_notifiers */

	old_end = old_addr + len;
	flush_cache_range(vma, old_addr, old_end);

	mmun_start = old_addr;
	mmun_end   = old_end;
	mmu_notifier_invalidate_range_start(vma->vm_mm, mmun_start, mmun_end);

	for (; old_addr < old_end; old_addr += extent, new_addr += extent) {
		cond_resched();
		next = (old_addr + PMD_SIZE) & PMD_MASK;
		/* even if next overflowed, extent below will be ok */
		extent = next - old_addr;
		if (extent > old_end - old_addr)
			extent = old_end - old_addr;
		old_pmd = get_old_pmd(vma->vm_mm, old_addr);
		if (!old_pmd)
			continue;
		new_pmd = alloc_new_pmd(vma->vm_mm, vma, new_addr);
		if (!new_pmd)
			break;
		if (pmd_trans_huge(*old_pmd)) {
			int err = 0;
			if (extent == HPAGE_PMD_SIZE) {
				VM_BUG_ON_VMA(vma->vm_file || !vma->anon_vma,
					      vma);
				/* See comment in move_ptes() */
				if (need_rmap_locks)
					anon_vma_lock_write(vma->anon_vma);
				err = move_huge_pmd(vma, new_vma, old_addr,
						    new_addr, old_end,
						    old_pmd, new_pmd);
				if (need_rmap_locks)
					anon_vma_unlock_write(vma->anon_vma);
					need_flush = true;
					continue;
			}
			if (err > 0) {
			}
			VM_BUG_ON(pmd_trans_huge(*old_pmd));
		}
		if (pmd_none(*new_pmd) && __pte_alloc(new_vma->vm_mm, new_vma,
						      new_pmd, new_addr))
			break;
		next = (new_addr + PMD_SIZE) & PMD_MASK;
		if (extent > next - new_addr)
			extent = next - new_addr;
		if (extent > LATENCY_LIMIT)
			extent = LATENCY_LIMIT;
		move_ptes(vma, old_pmd, old_addr, old_addr + extent,
			  new_vma, new_pmd, new_addr, need_rmap_locks);
		need_flush = true;
	}
	if (likely(need_flush))
		flush_tlb_range(vma, old_end-len, old_addr);

	mmu_notifier_invalidate_range_end(vma->vm_mm, mmun_start, mmun_end);

	return len + old_addr - old_end;	/* how much done */
}