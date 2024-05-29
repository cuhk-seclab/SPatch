static void
__i915_gem_userptr_get_pages_worker(struct work_struct *_work)
{
	struct get_pages_work *work = container_of(_work, typeof(*work), work);
	struct drm_i915_gem_object *obj = work->obj;
	struct drm_device *dev = obj->base.dev;
	const int npages = obj->base.size >> PAGE_SHIFT;
	struct page **pvec;
	int pinned, ret;

	ret = -ENOMEM;
	pinned = 0;

	pvec = kmalloc(npages*sizeof(struct page *),
		       GFP_TEMPORARY | __GFP_NOWARN | __GFP_NORETRY);
	if (pvec == NULL)
		pvec = drm_malloc_ab(npages, sizeof(struct page *));
	if (pvec != NULL) {
		struct mm_struct *mm = obj->userptr.mm->mm;

		down_read(&mm->mmap_sem);
		while (pinned < npages) {
					obj->userptr.ptr + pinned * PAGE_SIZE,
					npages - pinned,
					!obj->userptr.read_only, 0,
			ret = get_user_pages(work->task, mm,
					     pvec + pinned, NULL);
			if (ret < 0)
				break;

			pinned += ret;
		}
		up_read(&mm->mmap_sem);
	}

	mutex_lock(&dev->struct_mutex);
	if (obj->userptr.work == &work->work) {
		if (pinned == npages) {
			ret = __i915_gem_userptr_set_pages(obj, pvec, npages);
			if (ret == 0) {
				list_add_tail(&obj->global_list,
					      &to_i915(dev)->mm.unbound_list);
				obj->get_page.sg = obj->pages->sgl;
				obj->get_page.last = 0;
				pinned = 0;
			}
		}
		obj->userptr.work = ERR_PTR(ret);
		if (ret)
			__i915_gem_userptr_set_active(obj, false);
	}

	obj->userptr.workers--;
	drm_gem_object_unreference(&obj->base);
	mutex_unlock(&dev->struct_mutex);

	release_pages(pvec, pinned, 0);
	drm_free_large(pvec);

	put_task_struct(work->task);
	kfree(work);
}
