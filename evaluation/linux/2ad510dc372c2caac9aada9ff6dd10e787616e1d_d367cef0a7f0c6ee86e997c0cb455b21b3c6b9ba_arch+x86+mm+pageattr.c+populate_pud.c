static int populate_pud(struct cpa_data *cpa, unsigned long start, pgd_t *pgd,
			pgprot_t pgprot)
{
	pud_t *pud;
	unsigned long end;
	int cur_pages = 0;
	pgprot_t pud_pgprot;

	end = start + (cpa->numpages << PAGE_SHIFT);

	/*
	 * Not on a Gb page boundary? => map everything up to it with
	 * smaller pages.
	 */
	if (start & (PUD_SIZE - 1)) {
		unsigned long pre_end;
		unsigned long next_page = (start + PUD_SIZE) & PUD_MASK;

		pre_end   = min_t(unsigned long, end, next_page);
		cur_pages = (pre_end - start) >> PAGE_SHIFT;
		cur_pages = min_t(int, (int)cpa->numpages, cur_pages);

		pud = pud_offset(pgd, start);

		/*
		 * Need a PMD page?
		 */
		if (pud_none(*pud))
			if (alloc_pmd_page(pud))
				return -1;

		cur_pages = populate_pmd(cpa, start, pre_end, cur_pages,
					 pud, pgprot);
		if (cur_pages < 0)
			return cur_pages;

		start = pre_end;
	}

	/* We mapped them all? */
	if (cpa->numpages == cur_pages)
		return cur_pages;

	pud = pud_offset(pgd, start);
	pud_pgprot = pgprot_4k_2_large(pgprot);

	/*
	 * Map everything starting from the Gb boundary, possibly with 1G pages
	 */
	while (end - start >= PUD_SIZE) {
		set_pud(pud, __pud(cpa->pfn << PAGE_SHIFT | _PAGE_PSE |
				   massage_pgprot(pud_pgprot)));

		start	  += PUD_SIZE;
		cpa->pfn  += PUD_SIZE >> PAGE_SHIFT;
		cur_pages += PUD_SIZE >> PAGE_SHIFT;
		pud++;
	}

	/* Map trailing leftover */
	if (start < end) {
		int tmp;

		pud = pud_offset(pgd, start);
		if (pud_none(*pud))
			if (alloc_pmd_page(pud))
				return -1;

		tmp = populate_pmd(cpa, start, end, cpa->numpages - cur_pages,
				   pud, pgprot);
		if (tmp < 0)
			return cur_pages;

		cur_pages += tmp;
	}
	return cur_pages;
}
