int
nv40_instmem_new(struct nvkm_device *device, int index,
		 struct nvkm_instmem **pimem)
{
	struct nv40_instmem *imem;
	int bar;

	if (!(imem = kzalloc(sizeof(*imem), GFP_KERNEL)))
		return -ENOMEM;
	nvkm_instmem_ctor(&nv40_instmem, device, index, &imem->base);
	*pimem = &imem->base;

	/* map bar */
	if (device->func->resource_size(device, 2))
		bar = 2;
	else
		bar = 3;

	imem->iomem = ioremap(device->func->resource_addr(device, bar),
			      device->func->resource_size(device, bar));
	if (!imem->iomem) {
		nvkm_error(&imem->base.subdev, "unable to map PRAMIN BAR\n");
		return -EFAULT;
	}

	return 0;
}
