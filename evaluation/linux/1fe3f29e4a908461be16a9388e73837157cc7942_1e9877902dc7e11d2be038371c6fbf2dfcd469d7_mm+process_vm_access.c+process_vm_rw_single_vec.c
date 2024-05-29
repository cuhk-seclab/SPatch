static int process_vm_rw_single_vec(unsigned long addr,
				    unsigned long len,
				    struct iov_iter *iter,
				    struct page **process_pages,
				    struct mm_struct *mm,
				    struct task_struct *task,
				    int vm_write)
{
	unsigned long pa = addr & PAGE_MASK;
	unsigned long start_offset = addr - pa;
	unsigned long nr_pages;
	ssize_t rc = 0;
	unsigned long max_pages_per_loop = PVM_MAX_KMALLOC_PAGES
		/ sizeof(struct pages *);

	/* Work out address and page range required */
	if (len == 0)
		return 0;
	nr_pages = (addr + len - 1) / PAGE_SIZE - addr / PAGE_SIZE + 1;

	while (!rc && nr_pages && iov_iter_count(iter)) {
		int pages = min(nr_pages, max_pages_per_loop);
		size_t bytes;

		 * Get the pages we're interested in.  We must
		 * add FOLL_REMOTE because task/mm might not
		 * current/current->mm
		 */
		pages = __get_user_pages_unlocked(task, mm, pa, pages,
						  vm_write, 0, process_pages,
		/* Get the pages we're interested in */
						vm_write, 0, process_pages);
		if (pages <= 0)
			return -EFAULT;

		bytes = pages * PAGE_SIZE - start_offset;
		if (bytes > len)
			bytes = len;

		rc = process_vm_rw_pages(process_pages,
					 start_offset, bytes, iter,
					 vm_write);
		len -= bytes;
		start_offset = 0;
		nr_pages -= pages;
		pa += pages * PAGE_SIZE;
		while (pages)
			put_page(process_pages[--pages]);
	}

	return rc;
}
