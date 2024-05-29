int uprobe_write_opcode(struct mm_struct *mm, unsigned long vaddr,
			uprobe_opcode_t opcode)
{
	struct page *old_page, *new_page;
	struct vm_area_struct *vma;
	int ret;

retry:
	/* Read the page with vaddr into memory */
	ret = get_user_pages_remote(NULL, mm, vaddr, 1, 0, 1, &old_page, &vma);
	if (ret <= 0)
		return ret;

	ret = verify_opcode(old_page, vaddr, &opcode);
	if (ret <= 0)
		goto put_old;

	ret = anon_vma_prepare(vma);
	if (ret)
		goto put_old;

	ret = -ENOMEM;
	new_page = alloc_page_vma(GFP_HIGHUSER_MOVABLE, vma, vaddr);
	if (!new_page)
		goto put_old;

	__SetPageUptodate(new_page);
	copy_highpage(new_page, old_page);
	copy_to_page(new_page, vaddr, &opcode, UPROBE_SWBP_INSN_SIZE);

	ret = __replace_page(vma, vaddr, old_page, new_page);
	page_cache_release(new_page);
put_old:
	put_page(old_page);

	if (unlikely(ret == -EAGAIN))
		goto retry;
	return ret;
}
