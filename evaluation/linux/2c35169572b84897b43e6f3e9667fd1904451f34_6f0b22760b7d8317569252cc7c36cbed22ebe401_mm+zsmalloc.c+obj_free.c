static void obj_free(struct zs_pool *pool, struct size_class *class,
			unsigned long obj)
{
	struct link_free *link;
	struct page *first_page, *f_page;
	unsigned long f_objidx, f_offset;
	void *vaddr;

	BUG_ON(!obj);

	obj &= ~OBJ_ALLOCATED_TAG;
	obj_to_location(obj, &f_page, &f_objidx);
	first_page = get_first_page(f_page);

	f_offset = obj_idx_to_offset(f_page, f_objidx, class->size);

	vaddr = kmap_atomic(f_page);

	/* Insert this object in containing zspage's freelist */
	link = (struct link_free *)(vaddr + f_offset);
	link->next = first_page->freelist;
	if (class->huge)
		set_page_private(first_page, 0);
	kunmap_atomic(vaddr);
	first_page->freelist = (void *)obj;
	first_page->inuse--;
	zs_stat_dec(class, OBJ_USED, 1);
}
