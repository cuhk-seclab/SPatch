static dma_addr_t __map_single(struct device *dev,
			       struct dma_ops_domain *dma_dom,
			       phys_addr_t paddr,
			       size_t size,
			       int dir,
			       bool align,
			       u64 dma_mask)
{
	dma_addr_t offset = paddr & ~PAGE_MASK;
	dma_addr_t address, start, ret;
	unsigned int pages;
	unsigned long align_mask = 0;
	int i;

	pages = iommu_num_pages(paddr, size, PAGE_SIZE);
	paddr &= PAGE_MASK;

	INC_STATS_COUNTER(total_map_requests);

	if (pages > 1)
		INC_STATS_COUNTER(cross_page);

	if (align)
		align_mask = (1UL << get_order(size)) - 1;

retry:
	address = dma_ops_alloc_addresses(dev, dma_dom, pages, align_mask,
					  dma_mask);
	if (unlikely(address == DMA_ERROR_CODE)) {
		if (alloc_new_range(dma_dom, false, GFP_ATOMIC))
			goto out;

		/*
		 * setting next_index here will let the address
		 * allocator only scan the new allocated range in the
		 * first run. This is a small optimization.
		 */
		dma_dom->next_index = dma_dom->aperture_size >> APERTURE_RANGE_SHIFT;

		/*
		 * aperture was successfully enlarged by 128 MB, try
		 * allocation again
		 */
		goto retry;
	}

	start = address;
	for (i = 0; i < pages; ++i) {
		ret = dma_ops_domain_map(dma_dom, start, paddr, dir);
		if (ret == DMA_ERROR_CODE)
			goto out_unmap;

		paddr += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	address += offset;

	ADD_STATS_COUNTER(alloced_io_mem, size);

	if (unlikely(amd_iommu_np_cache)) {
		domain_flush_pages(&dma_dom->domain, address, size);
		domain_flush_complete(&dma_dom->domain);
	}

out:
	return address;

out_unmap:

	for (--i; i >= 0; --i) {
		start -= PAGE_SIZE;
		dma_ops_domain_unmap(dma_dom, start);
	}

	dma_ops_free_addresses(dma_dom, address, pages);

	return DMA_ERROR_CODE;
}
