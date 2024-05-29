void
nv04_mmu_dtor(struct nvkm_object *object)
{
	struct nv04_mmu *mmu = (void *)object;
	if (mmu->vm) {
		nvkm_gpuobj_ref(NULL, &mmu->vm->pgt[0].obj[0]);
		nvkm_vm_ref(NULL, &mmu->vm, NULL);
	}
	if (mmu->nullp) {
		pci_free_consistent(nv_device(mmu)->pdev, 16 * 1024,
				    mmu->nullp, mmu->null);
	}
	nvkm_mmu_destroy(&mmu->base);
}
