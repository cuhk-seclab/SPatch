static struct ldt_struct *alloc_ldt_struct(int size)
{
	struct ldt_struct *new_ldt;
	int alloc_size;

	if (size > LDT_ENTRIES)
		return NULL;

	new_ldt = kmalloc(sizeof(struct ldt_struct), GFP_KERNEL);
	if (!new_ldt)
		return NULL;

	BUILD_BUG_ON(LDT_ENTRY_SIZE != sizeof(struct desc_struct));
	alloc_size = size * LDT_ENTRY_SIZE;

	/*
	 * Xen is very picky: it requires a page-aligned LDT that has no
	 * trailing nonzero bytes in any page that contains LDT descriptors.
	 * Keep it simple: zero the whole allocation and never allocate less
	 * than PAGE_SIZE.
	 */
	if (alloc_size > PAGE_SIZE)
		new_ldt->entries = vzalloc(alloc_size);
	else
		new_ldt->entries = (void *)get_zeroed_page(GFP_KERNEL);

	if (!new_ldt->entries) {
		kfree(new_ldt);
		return NULL;
	}

	new_ldt->size = size;
	return new_ldt;
}
