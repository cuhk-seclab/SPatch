int
_nvkm_pmu_init(struct nvkm_object *object)
{
	const struct nvkm_pmu_impl *impl = (void *)object->oclass;
	struct nvkm_pmu *pmu = (void *)object;
	int ret, i;

	ret = nvkm_subdev_init(&pmu->subdev);
	if (ret)
		return ret;

	nv_subdev(pmu)->intr = nvkm_pmu_intr;
	pmu->message = nvkm_pmu_send;
	pmu->pgob = nvkm_pmu_pgob;

	/* prevent previous ucode from running, wait for idle, reset */
	nv_wr32(pmu, 0x10a014, 0x0000ffff); /* INTR_EN_CLR = ALL */
	nv_wait(pmu, 0x10a04c, 0xffffffff, 0x00000000);
	nv_mask(pmu, 0x000200, 0x00002000, 0x00000000);
	nv_mask(pmu, 0x000200, 0x00002000, 0x00002000);
	nv_rd32(pmu, 0x000200);
	nv_wait(pmu, 0x10a10c, 0x00000006, 0x00000000);

	/* upload data segment */
	nv_wr32(pmu, 0x10a1c0, 0x01000000);
	for (i = 0; i < impl->data.size / 4; i++)
		nv_wr32(pmu, 0x10a1c4, impl->data.data[i]);

	/* upload code segment */
	nv_wr32(pmu, 0x10a180, 0x01000000);
	for (i = 0; i < impl->code.size / 4; i++) {
		if ((i & 0x3f) == 0)
			nv_wr32(pmu, 0x10a188, i >> 6);
		nv_wr32(pmu, 0x10a184, impl->code.data[i]);
	}

	/* start it running */
	nv_wr32(pmu, 0x10a10c, 0x00000000);
	nv_wr32(pmu, 0x10a104, 0x00000000);
	nv_wr32(pmu, 0x10a100, 0x00000002);

	/* wait for valid host->pmu ring configuration */
	if (!nv_wait_ne(pmu, 0x10a4d0, 0xffffffff, 0x00000000))
		return -EBUSY;
	pmu->send.base = nv_rd32(pmu, 0x10a4d0) & 0x0000ffff;
	pmu->send.size = nv_rd32(pmu, 0x10a4d0) >> 16;

	/* wait for valid pmu->host ring configuration */
	if (!nv_wait_ne(pmu, 0x10a4dc, 0xffffffff, 0x00000000))
		return -EBUSY;
	pmu->recv.base = nv_rd32(pmu, 0x10a4dc) & 0x0000ffff;
	pmu->recv.size = nv_rd32(pmu, 0x10a4dc) >> 16;

	nv_wr32(pmu, 0x10a010, 0x000000e0);
	return 0;
}
