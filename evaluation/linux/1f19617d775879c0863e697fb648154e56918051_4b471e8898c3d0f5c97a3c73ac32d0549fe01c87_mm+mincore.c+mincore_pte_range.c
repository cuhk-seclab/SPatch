static int mincore_pte_range(pmd_t *pmd, unsigned long addr, unsigned long end,
			struct mm_walk *walk)
{
	spinlock_t *ptl;
	struct vm_area_struct *vma = walk->vma;
	pte_t *ptep;
	unsigned char *vec = walk->private;
	int nr = (end - addr) >> PAGE_SHIFT;

	if (pmd_trans_huge_lock(pmd, vma, &ptl)) {
		memset(vec, 1, nr);
		spin_unlock(ptl);
		goto out;
	}

	if (pmd_trans_unstable(pmd)) {
		__mincore_unmapped_range(addr, end, vma, vec);
		goto out;
	}

	ptep = pte_offset_map_lock(walk->mm, pmd, addr, &ptl);
	for (; addr != end; ptep++, addr += PAGE_SIZE) {
		pte_t pte = *ptep;

		if (pte_none(pte))
			__mincore_unmapped_range(addr, addr + PAGE_SIZE,
						 vma, vec);
		else if (pte_present(pte))
			*vec = 1;
		else { /* pte is a swap entry */
			swp_entry_t entry = pte_to_swp_entry(pte);

			if (non_swap_entry(entry)) {
				/*
				 * migration or hwpoison entries are always
				 * uptodate
				 */
				*vec = 1;
			} else {
#ifdef CONFIG_SWAP
				*vec = mincore_page(swap_address_space(entry),
					entry.val);
#else
				WARN_ON(1);
				*vec = 1;
#endif
			}
		}
		vec++;
	}
	pte_unmap_unlock(ptep - 1, ptl);
out:
	walk->private += nr;
	cond_resched();
	return 0;
}
