static int
nv04_gart_manager_init(struct ttm_mem_type_manager *man, unsigned long psize)
{
	struct nouveau_drm *drm = nouveau_bdev(man->bdev);
	struct nvkm_mmu *mmu = nvxx_mmu(&drm->device);
	struct nv04_mmu *priv = (void *)mmu;
	struct nvkm_vm *vm = NULL;
	nvkm_vm_ref(priv->vm, &vm, NULL);
	man->priv = vm;
	return 0;
}
