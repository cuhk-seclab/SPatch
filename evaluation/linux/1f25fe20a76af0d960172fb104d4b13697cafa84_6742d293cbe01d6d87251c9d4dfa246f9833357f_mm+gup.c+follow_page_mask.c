struct page *follow_page_mask(struct vm_area_struct *vma,
			      unsigned long address, unsigned int flags,
			      unsigned int *page_mask)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	spinlock_t *ptl;
	struct page *page;
	struct mm_struct *mm = vma->vm_mm;

	*page_mask = 0;

	page = follow_huge_addr(mm, address, flags & FOLL_WRITE);
	if (!IS_ERR(page)) {
		BUG_ON(flags & FOLL_GET);
		return page;
	}

	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
		return no_page_table(vma, flags);

	pud = pud_offset(pgd, address);
	if (pud_none(*pud))
		return no_page_table(vma, flags);
	if (pud_huge(*pud) && vma->vm_flags & VM_HUGETLB) {
		page = follow_huge_pud(mm, address, pud, flags);
		if (page)
			return page;
		return no_page_table(vma, flags);
	}
	if (unlikely(pud_bad(*pud)))
		return no_page_table(vma, flags);

	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd))
		return no_page_table(vma, flags);
	if (pmd_huge(*pmd) && vma->vm_flags & VM_HUGETLB) {
		page = follow_huge_pmd(mm, address, pmd, flags);
		if (page)
			return page;
		return no_page_table(vma, flags);
	}
	if ((flags & FOLL_NUMA) && pmd_protnone(*pmd))
		return no_page_table(vma, flags);
	if (likely(!pmd_trans_huge(*pmd)))
		return follow_page_pte(vma, address, pmd, flags);

	ptl = pmd_lock(mm, pmd);
	if (unlikely(!pmd_trans_huge(*pmd))) {
		spin_unlock(ptl);
		return follow_page_pte(vma, address, pmd, flags);
	}

	if (unlikely(pmd_trans_splitting(*pmd))) {
		spin_unlock(ptl);
		wait_split_huge_page(vma->anon_vma, pmd);
		return follow_page_pte(vma, address, pmd, flags);
	}

	if (flags & FOLL_SPLIT) {
		int ret;
		page = pmd_page(*pmd);
		if (is_huge_zero_page(page)) {
			spin_unlock(ptl);
			ret = 0;
			split_huge_page_pmd(vma, address, pmd);
		} else {
			get_page(page);
			spin_unlock(ptl);
			lock_page(page);
			ret = split_huge_page(page);
			unlock_page(page);
			put_page(page);
		}

		return ret ? ERR_PTR(ret) :
			follow_page_pte(vma, address, pmd, flags);
	}

	page = follow_trans_huge_pmd(vma, address, pmd, flags);
	spin_unlock(ptl);
	*page_mask = HPAGE_PMD_NR - 1;
	return page;
}
