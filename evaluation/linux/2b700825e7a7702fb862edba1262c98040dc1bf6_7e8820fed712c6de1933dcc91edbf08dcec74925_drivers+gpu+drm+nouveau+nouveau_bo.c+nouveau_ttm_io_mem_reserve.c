static int
nouveau_ttm_io_mem_reserve(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem)
{
	struct ttm_mem_type_manager *man = &bdev->man[mem->mem_type];
	struct nouveau_drm *drm = nouveau_bdev(bdev);
	struct nvkm_device *device = nvxx_device(&drm->device);
	struct nvkm_mem *node = mem->mm_node;
	int ret;

	mem->bus.addr = NULL;
	mem->bus.offset = 0;
	mem->bus.size = mem->num_pages << PAGE_SHIFT;
	mem->bus.base = 0;
	mem->bus.is_iomem = false;
	if (!(man->flags & TTM_MEMTYPE_FLAG_MAPPABLE))
		return -EINVAL;
	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
		/* System memory */
		return 0;
	case TTM_PL_TT:
#if __OS_HAS_AGP
		if (drm->agp.stat == ENABLED) {
			mem->bus.offset = mem->start << PAGE_SHIFT;
			mem->bus.base = drm->agp.base;
			mem->bus.is_iomem = !drm->dev->agp->cant_use_aperture;
		}
#endif
		if (drm->device.info.family < NV_DEVICE_INFO_V0_TESLA || !node->memtype)
			/* untiled */
			break;
		/* fallthrough, tiled memory */
	case TTM_PL_VRAM:
		mem->bus.offset = mem->start << PAGE_SHIFT;
		mem->bus.base = device->func->resource_addr(device, 1);
		mem->bus.is_iomem = true;
		if (drm->device.info.family >= NV_DEVICE_INFO_V0_TESLA) {
			struct nvkm_bar *bar = nvxx_bar(&drm->device);
			int page_shift = 12;
			if (drm->device.info.family >= NV_DEVICE_INFO_V0_FERMI)
				page_shift = node->page_shift;

			ret = nvkm_bar_umap(bar, node->size << 12, page_shift,
					    &node->bar_vma);
			if (ret)
				return ret;

			nvkm_vm_map(&node->bar_vma, node);
			mem->bus.offset = node->bar_vma.offset;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
