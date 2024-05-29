static void gen8_ggtt_insert_entries(struct i915_address_space *vm,
				     struct sg_table *st,
				     uint64_t start,
				     enum i915_cache_level level, u32 unused)
{
	struct drm_i915_private *dev_priv = vm->dev->dev_private;
	unsigned first_entry = start >> PAGE_SHIFT;
	gen8_pte_t __iomem *gtt_entries =
		(gen8_pte_t __iomem *)dev_priv->gtt.gsm + first_entry;
	int i = 0;
	struct sg_page_iter sg_iter;
	dma_addr_t addr = 0; /* shut up gcc */
	int rpm_atomic_seq;

	rpm_atomic_seq = assert_rpm_atomic_begin(dev_priv);

	for_each_sg_page(st->sgl, &sg_iter, st->nents, 0) {
		addr = sg_dma_address(sg_iter.sg) +
			(sg_iter.sg_pgoffset << PAGE_SHIFT);
		gen8_set_pte(&gtt_entries[i],
			     gen8_pte_encode(addr, level, true));
		i++;
	}

	/*
	 * XXX: This serves as a posting read to make sure that the PTE has
	 * actually been updated. There is some concern that even though
	 * registers and PTEs are within the same BAR that they are potentially
	 * of NUMA access patterns. Therefore, even with the way we assume
	 * hardware should work, we must keep this posting read for paranoia.
	 */
	if (i != 0)
		WARN_ON(readq(&gtt_entries[i-1])
			!= gen8_pte_encode(addr, level, true));

	/* This next bit makes the above posting read even more important. We
	 * want to flush the TLBs only after we're certain all the PTE updates
	 * have finished.
	 */
	I915_WRITE(GFX_FLSH_CNTL_GEN6, GFX_FLSH_CNTL_EN);
	POSTING_READ(GFX_FLSH_CNTL_GEN6);

	assert_rpm_atomic_end(dev_priv, rpm_atomic_seq);
}
