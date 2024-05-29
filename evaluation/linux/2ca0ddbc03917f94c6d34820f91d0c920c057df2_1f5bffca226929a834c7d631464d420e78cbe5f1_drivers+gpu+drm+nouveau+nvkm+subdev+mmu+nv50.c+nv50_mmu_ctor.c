static int
nv50_mmu_ctor(struct nvkm_object *parent, struct nvkm_object *engine,
	      struct nvkm_oclass *oclass, void *data, u32 size,
	      struct nvkm_object **pobject)
{
	struct nv50_mmu_priv *priv;
	int ret;

	ret = nvkm_mmu_create(parent, engine, oclass, "VM", "vm", &priv);
	*pobject = nv_object(priv);
	if (ret)
		return ret;

	mmu->dma_bits = 40;
	mmu->pgt_bits  = 29 - 12;
	mmu->spg_shift = 12;
	mmu->lpg_shift = 16;
	mmu->create = nv50_vm_create;
	mmu->map_pgt = nv50_vm_map_pgt;
	mmu->map = nv50_vm_map;
	mmu->map_sg = nv50_vm_map_sg;
	mmu->unmap = nv50_vm_unmap;
	priv->base.limit = 1ULL << 40;
	priv->base.flush = nv50_vm_flush;
	return 0;
}
