static void zs_object_copy(unsigned long src, unsigned long dst,
				struct size_class *class)
{
	struct page *s_page, *d_page;
	unsigned long s_objidx, d_objidx;
	unsigned long s_off, d_off;
	void *s_addr, *d_addr;
	int s_size, d_size, size;
	int written = 0;

	s_size = d_size = class->size;

	obj_to_location(src, &s_page, &s_objidx);
	obj_to_location(dst, &d_page, &d_objidx);

	s_off = obj_idx_to_offset(s_page, s_objidx, class->size);
	d_off = obj_idx_to_offset(d_page, d_objidx, class->size);

	if (s_off + class->size > PAGE_SIZE)
		s_size = PAGE_SIZE - s_off;

	if (d_off + class->size > PAGE_SIZE)
		d_size = PAGE_SIZE - d_off;

	s_addr = kmap_atomic(s_page);
	d_addr = kmap_atomic(d_page);

	while (1) {
		size = min(s_size, d_size);
		memcpy(d_addr + d_off, s_addr + s_off, size);
		written += size;

		if (written == class->size)
			break;

		s_size -= size;
		d_off += size;
		d_size -= size;

		if (s_off + size >= PAGE_SIZE) {
			kunmap_atomic(d_addr);
			kunmap_atomic(s_addr);
			s_page = get_next_page(s_page);
			BUG_ON(!s_page);
			s_addr = kmap_atomic(s_page);
			d_addr = kmap_atomic(d_page);
			s_size = class->size - written;
			s_off = 0;
		} else {
			s_size -= size;
		}

		if (d_off + size >= PAGE_SIZE) {
			kunmap_atomic(d_addr);
			d_page = get_next_page(d_page);
			BUG_ON(!d_page);
			d_addr = kmap_atomic(d_page);
			d_size = class->size - written;
			d_off = 0;
		} else {
			d_size -= size;
		}
	}

	kunmap_atomic(d_addr);
	kunmap_atomic(s_addr);
}
