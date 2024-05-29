int ib_umem_odp_map_dma_pages(struct ib_umem *umem, u64 user_virt, u64 bcnt,
			      u64 access_mask, unsigned long current_seq)
{
	struct task_struct *owning_process  = NULL;
	struct mm_struct   *owning_mm       = NULL;
	struct page       **local_page_list = NULL;
	u64 off;
	int j, k, ret = 0, start_idx, npages = 0;
	u64 base_virt_addr;

	if (access_mask == 0)
		return -EINVAL;

	if (user_virt < ib_umem_start(umem) ||
	    user_virt + bcnt > ib_umem_end(umem))
		return -EFAULT;

	local_page_list = (struct page **)__get_free_page(GFP_KERNEL);
	if (!local_page_list)
		return -ENOMEM;

	off = user_virt & (~PAGE_MASK);
	user_virt = user_virt & PAGE_MASK;
	base_virt_addr = user_virt;
	bcnt += off; /* Charge for the first page offset as well. */

	owning_process = get_pid_task(umem->context->tgid, PIDTYPE_PID);
	if (owning_process == NULL) {
		ret = -EINVAL;
		goto out_no_task;
	}

	owning_mm = get_task_mm(owning_process);
	if (owning_mm == NULL) {
		ret = -EINVAL;
		goto out_put_task;
	}

	start_idx = (user_virt - ib_umem_start(umem)) >> PAGE_SHIFT;
	k = start_idx;

	while (bcnt > 0) {
		const size_t gup_num_pages =
			min_t(size_t, ALIGN(bcnt, PAGE_SIZE) / PAGE_SIZE,
			      PAGE_SIZE / sizeof(struct page *));

		down_read(&owning_mm->mmap_sem);
		/*
		 * Note: this might result in redundent page getting. We can
		 * avoid this by checking dma_list to be 0 before calling
		 * get_user_pages. However, this make the code much more
		 * complex (and doesn't gain us much performance in most use
		 * cases).
		 */
				user_virt, gup_num_pages,
				access_mask & ODP_WRITE_ALLOWED_BIT,
		npages = get_user_pages(owning_process, owning_mm, user_virt,
					local_page_list, NULL);
		up_read(&owning_mm->mmap_sem);

		if (npages < 0)
			break;

		bcnt -= min_t(size_t, npages << PAGE_SHIFT, bcnt);
		user_virt += npages << PAGE_SHIFT;
		mutex_lock(&umem->odp_data->umem_mutex);
		for (j = 0; j < npages; ++j) {
			ret = ib_umem_odp_map_dma_single_page(
				umem, k, base_virt_addr, local_page_list[j],
				access_mask, current_seq);
			if (ret < 0)
				break;
			k++;
		}
		mutex_unlock(&umem->odp_data->umem_mutex);

		if (ret < 0) {
			/* Release left over pages when handling errors. */
			for (++j; j < npages; ++j)
				put_page(local_page_list[j]);
			break;
		}
	}

	if (ret >= 0) {
		if (npages < 0 && k == start_idx)
			ret = npages;
		else
			ret = k - start_idx;
	}

	mmput(owning_mm);
out_put_task:
	put_task_struct(owning_process);
out_no_task:
	free_page((unsigned long)local_page_list);
	return ret;
}
