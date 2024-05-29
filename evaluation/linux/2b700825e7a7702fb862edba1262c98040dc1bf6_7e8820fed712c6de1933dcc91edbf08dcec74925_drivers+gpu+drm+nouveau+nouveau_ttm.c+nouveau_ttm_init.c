int
nouveau_ttm_init(struct nouveau_drm *drm)
{
	struct nvkm_device *device = nvxx_device(&drm->device);
	struct drm_device *dev = drm->dev;
	u32 bits;
	int ret;

	bits = nvxx_mmu(&drm->device)->dma_bits;
	if (nv_device_is_pci(nvxx_device(&drm->device))) {
		if (drm->agp.stat == ENABLED ||
		     !pci_dma_supported(dev->pdev, DMA_BIT_MASK(bits)))
			bits = 32;

		ret = pci_set_dma_mask(dev->pdev, DMA_BIT_MASK(bits));
		if (ret)
			return ret;

		ret = pci_set_consistent_dma_mask(dev->pdev,
						  DMA_BIT_MASK(bits));
		if (ret)
			pci_set_consistent_dma_mask(dev->pdev,
						    DMA_BIT_MASK(32));
	}

	ret = nouveau_ttm_global_init(drm);
	if (ret)
		return ret;

	ret = ttm_bo_device_init(&drm->ttm.bdev,
				  drm->ttm.bo_global_ref.ref.object,
				  &nouveau_bo_driver,
				  dev->anon_inode->i_mapping,
				  DRM_FILE_PAGE_OFFSET,
				  bits <= 32 ? true : false);
	if (ret) {
		NV_ERROR(drm, "error initialising bo driver, %d\n", ret);
		return ret;
	}

	/* VRAM init */
	drm->gem.vram_available = drm->device.info.ram_user;

	ret = ttm_bo_init_mm(&drm->ttm.bdev, TTM_PL_VRAM,
			      drm->gem.vram_available >> PAGE_SHIFT);
	if (ret) {
		NV_ERROR(drm, "VRAM mm init failed, %d\n", ret);
		return ret;
	}

	drm->ttm.mtrr = arch_phys_wc_add(device->func->resource_addr(device, 1),
					 device->func->resource_size(device, 1));

	/* GART init */
	if (drm->agp.stat != ENABLED) {
		drm->gem.gart_available = nvxx_mmu(&drm->device)->limit;
	} else {
		drm->gem.gart_available = drm->agp.size;
	}

	ret = ttm_bo_init_mm(&drm->ttm.bdev, TTM_PL_TT,
			      drm->gem.gart_available >> PAGE_SHIFT);
	if (ret) {
		NV_ERROR(drm, "GART mm init failed, %d\n", ret);
		return ret;
	}

	NV_INFO(drm, "VRAM: %d MiB\n", (u32)(drm->gem.vram_available >> 20));
	NV_INFO(drm, "GART: %d MiB\n", (u32)(drm->gem.gart_available >> 20));
	return 0;
}
