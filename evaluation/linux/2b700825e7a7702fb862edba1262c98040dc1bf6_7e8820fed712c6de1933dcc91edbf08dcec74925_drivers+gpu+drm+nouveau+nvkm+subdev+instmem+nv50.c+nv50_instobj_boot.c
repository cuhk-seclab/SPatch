static void
nv50_instobj_boot(struct nvkm_memory *memory, struct nvkm_vm *vm)
{
	struct nv50_instobj *iobj = nv50_instobj(memory);
	struct nvkm_subdev *subdev = &iobj->imem->base.subdev;
	struct nvkm_device *device = subdev->device;
	u64 size = nvkm_memory_size(memory);
	void __iomem *map;
	int ret;

	iobj->map = ERR_PTR(-ENOMEM);

	ret = nvkm_vm_get(vm, size, 12, NV_MEM_ACCESS_RW, &iobj->bar);
	if (ret == 0) {
		map = ioremap(device->func->resource_addr(device, 3) +
			      (u32)iobj->bar.offset, size);
		if (map) {
			nvkm_memory_map(memory, &iobj->bar, 0);
			iobj->map = map;
		} else {
			nvkm_warn(subdev, "PRAMIN ioremap failed\n");
			nvkm_vm_put(&iobj->bar);
		}
	} else {
		nvkm_warn(subdev, "PRAMIN exhausted\n");
	}
}
