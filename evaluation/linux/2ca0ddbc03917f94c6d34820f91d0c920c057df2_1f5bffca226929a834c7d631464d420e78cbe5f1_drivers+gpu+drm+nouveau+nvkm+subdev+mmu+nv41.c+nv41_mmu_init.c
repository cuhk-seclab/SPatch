static int
nv41_mmu_init(struct nvkm_object *object)
{
	struct nv04_mmu *mmu = (void *)object;
	struct nvkm_gpuobj *dma = mmu->vm->pgt[0].obj[0];
	int ret;

	ret = nvkm_mmu_init(&mmu->base);
	if (ret)
		return ret;

	nv_wr32(mmu, 0x100800, dma->addr | 0x00000002);
	nv_mask(mmu, 0x10008c, 0x00000100, 0x00000100);
	nv_wr32(mmu, 0x100820, 0x00000000);
	return 0;
}
