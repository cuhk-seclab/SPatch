static int
nv04_dmaobj_bind(struct nvkm_dmaobj *dmaobj, struct nvkm_object *parent,
		 struct nvkm_gpuobj **pgpuobj)
{
	struct nv04_dmaobj_priv *priv = (void *)dmaobj;
	struct nvkm_gpuobj *gpuobj;
	u64 offset = priv->base.start & 0xfffff000;
	u64 adjust = priv->base.start & 0x00000fff;
	u32 length = priv->base.limit - priv->base.start;
	int ret;

	if (!nv_iclass(parent, NV_ENGCTX_CLASS)) {
		switch (nv_mclass(parent->parent)) {
		case NV03_CHANNEL_DMA:
		case NV10_CHANNEL_DMA:
		case NV17_CHANNEL_DMA:
		case NV40_CHANNEL_DMA:
			break;
		default:
			return -EINVAL;
		}
	}

	if (priv->clone) {
		struct nv04_mmu *mmu = nv04_mmu(dmaobj);
		struct nvkm_gpuobj *pgt = mmu->vm->pgt[0].obj[0];
		if (!dmaobj->start)
			return nvkm_gpuobj_dup(parent, pgt, pgpuobj);
		offset  = nv_ro32(pgt, 8 + (offset >> 10));
		offset &= 0xfffff000;
	}

	ret = nvkm_gpuobj_new(parent, parent, 16, 16, 0, &gpuobj);
	*pgpuobj = gpuobj;
	if (ret == 0) {
		nv_wo32(*pgpuobj, 0x00, priv->flags0 | (adjust << 20));
		nv_wo32(*pgpuobj, 0x04, length);
		nv_wo32(*pgpuobj, 0x08, priv->flags2 | offset);
		nv_wo32(*pgpuobj, 0x0c, priv->flags2 | offset);
	}

	return ret;
}
