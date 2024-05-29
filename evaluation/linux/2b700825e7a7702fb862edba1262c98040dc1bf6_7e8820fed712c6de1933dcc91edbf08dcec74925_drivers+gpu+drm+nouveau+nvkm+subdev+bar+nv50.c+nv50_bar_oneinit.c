int
nv50_bar_oneinit(struct nvkm_bar *base)
{
	struct nv50_bar *bar = nv50_bar(base);
	struct nvkm_device *device = bar->base.subdev.device;
	static struct lock_class_key bar1_lock;
	static struct lock_class_key bar3_lock;
	struct nvkm_vm *vm;
	u64 start, limit;
	int ret;

	ret = nvkm_gpuobj_new(device, 0x20000, 0, false, NULL, &bar->mem);
	if (ret)
		return ret;

	ret = nvkm_gpuobj_new(device, bar->pgd_addr, 0, false, bar->mem,
			      &bar->pad);
	if (ret)
		return ret;

	ret = nvkm_gpuobj_new(device, 0x4000, 0, false, bar->mem, &bar->pgd);
	if (ret)
		return ret;

	/* BAR3 */
	start = 0x0100000000ULL;
	limit = start + device->func->resource_size(device, 3);

	ret = nvkm_vm_new(device, start, limit, start, &bar3_lock, &vm);
	if (ret)
		return ret;

	atomic_inc(&vm->engref[NVKM_SUBDEV_BAR]);

	ret = nvkm_vm_boot(vm, limit-- - start);
	if (ret)
		return ret;

	ret = nvkm_vm_ref(vm, &bar->bar3_vm, bar->pgd);
	nvkm_vm_ref(NULL, &vm, NULL);
	if (ret)
		return ret;

	ret = nvkm_gpuobj_new(device, 24, 16, false, bar->mem, &bar->bar3);
	if (ret)
		return ret;

	nvkm_kmap(bar->bar3);
	nvkm_wo32(bar->bar3, 0x00, 0x7fc00000);
	nvkm_wo32(bar->bar3, 0x04, lower_32_bits(limit));
	nvkm_wo32(bar->bar3, 0x08, lower_32_bits(start));
	nvkm_wo32(bar->bar3, 0x0c, upper_32_bits(limit) << 24 |
				   upper_32_bits(start));
	nvkm_wo32(bar->bar3, 0x10, 0x00000000);
	nvkm_wo32(bar->bar3, 0x14, 0x00000000);
	nvkm_done(bar->bar3);

	/* BAR1 */
	start = 0x0000000000ULL;
	limit = start + device->func->resource_size(device, 1);

	ret = nvkm_vm_new(device, start, limit--, start, &bar1_lock, &vm);
	if (ret)
		return ret;

	atomic_inc(&vm->engref[NVKM_SUBDEV_BAR]);

	ret = nvkm_vm_ref(vm, &bar->bar1_vm, bar->pgd);
	nvkm_vm_ref(NULL, &vm, NULL);
	if (ret)
		return ret;

	ret = nvkm_gpuobj_new(device, 24, 16, false, bar->mem, &bar->bar1);
	if (ret)
		return ret;

	nvkm_kmap(bar->bar1);
	nvkm_wo32(bar->bar1, 0x00, 0x7fc00000);
	nvkm_wo32(bar->bar1, 0x04, lower_32_bits(limit));
	nvkm_wo32(bar->bar1, 0x08, lower_32_bits(start));
	nvkm_wo32(bar->bar1, 0x0c, upper_32_bits(limit) << 24 |
				   upper_32_bits(start));
	nvkm_wo32(bar->bar1, 0x10, 0x00000000);
	nvkm_wo32(bar->bar1, 0x14, 0x00000000);
	nvkm_done(bar->bar1);
	return 0;
}
