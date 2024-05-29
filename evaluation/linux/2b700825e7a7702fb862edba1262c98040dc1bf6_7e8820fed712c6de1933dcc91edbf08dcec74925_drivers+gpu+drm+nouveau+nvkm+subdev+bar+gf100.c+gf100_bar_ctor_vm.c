static int
gf100_bar_ctor_vm(struct gf100_bar *bar, struct gf100_bar_vm *bar_vm,
		  struct lock_class_key *key, int bar_nr)
{
	struct nvkm_device *device = bar->base.subdev.device;
	struct nvkm_vm *vm;
	resource_size_t bar_len;
	int ret;

	ret = nvkm_memory_new(device, NVKM_MEM_TARGET_INST, 0x1000, 0, false,
			      &bar_vm->mem);
	if (ret)
		return ret;

	ret = nvkm_gpuobj_new(device, 0x8000, 0, false, NULL, &bar_vm->pgd);
	if (ret)
		return ret;

	bar_len = device->func->resource_size(device, bar_nr);

	ret = nvkm_vm_new(device, 0, bar_len, 0, key, &vm);
	if (ret)
		return ret;

	atomic_inc(&vm->engref[NVKM_SUBDEV_BAR]);

	/*
	 * Bootstrap page table lookup.
	 */
	if (bar_nr == 3) {
		ret = nvkm_vm_boot(vm, bar_len);
		if (ret) {
			nvkm_vm_ref(NULL, &vm, NULL);
			return ret;
		}
	}

	ret = nvkm_vm_ref(vm, &bar_vm->vm, bar_vm->pgd);
	nvkm_vm_ref(NULL, &vm, NULL);
	if (ret)
		return ret;

	nvkm_kmap(bar_vm->mem);
	nvkm_wo32(bar_vm->mem, 0x0200, lower_32_bits(bar_vm->pgd->addr));
	nvkm_wo32(bar_vm->mem, 0x0204, upper_32_bits(bar_vm->pgd->addr));
	nvkm_wo32(bar_vm->mem, 0x0208, lower_32_bits(bar_len - 1));
	nvkm_wo32(bar_vm->mem, 0x020c, upper_32_bits(bar_len - 1));
	nvkm_done(bar_vm->mem);
	return 0;
}
