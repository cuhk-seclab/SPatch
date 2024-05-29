static int
nouveau_channel_prep(struct nouveau_drm *drm, struct nvif_device *device,
		     u32 handle, u32 size, struct nouveau_channel **pchan)
{
	struct nouveau_cli *cli = (void *)device->object.client;
	struct nvkm_mmu *mmu = nvxx_mmu(device);
	struct nv_dma_v0 args = {};
	struct nouveau_channel *chan;
	u32 target;
	int ret;

	chan = *pchan = kzalloc(sizeof(*chan), GFP_KERNEL);
	if (!chan)
		return -ENOMEM;

	chan->device = device;
	chan->drm = drm;

	/* allocate memory for dma push buffer */
	target = TTM_PL_FLAG_TT | TTM_PL_FLAG_UNCACHED;
	if (nouveau_vram_pushbuf)
		target = TTM_PL_FLAG_VRAM;

	ret = nouveau_bo_new(drm->dev, size, 0, target, 0, 0, NULL, NULL,
			    &chan->push.buffer);
	if (ret == 0) {
		ret = nouveau_bo_pin(chan->push.buffer, target, false);
		if (ret == 0)
			ret = nouveau_bo_map(chan->push.buffer);
	}

	if (ret) {
		nouveau_channel_del(pchan);
		return ret;
	}

	/* create dma object covering the *entire* memory space that the
	 * pushbuf lives in, this is because the GEM code requires that
	 * we be able to call out to other (indirect) push buffers
	 */
	chan->push.vma.offset = chan->push.buffer->bo.offset;

	if (device->info.family >= NV_DEVICE_INFO_V0_TESLA) {
		ret = nouveau_bo_vma_add(chan->push.buffer, cli->vm,
					&chan->push.vma);
		if (ret) {
			nouveau_channel_del(pchan);
			return ret;
		}

		args.target = NV_DMA_V0_TARGET_VM;
		args.access = NV_DMA_V0_ACCESS_VM;
		args.start = 0;
		args.limit = cli->vm->mmu->limit - 1;
	} else
	if (chan->push.buffer->bo.mem.mem_type == TTM_PL_VRAM) {
		if (device->info.family == NV_DEVICE_INFO_V0_TNT) {
			/* nv04 vram pushbuf hack, retarget to its location in
			 * the framebuffer bar rather than direct vram access..
			 * nfi why this exists, it came from the -nv ddx.
			 */
			args.target = NV_DMA_V0_TARGET_PCI;
			args.access = NV_DMA_V0_ACCESS_RDWR;
			args.start = nvxx_device(device)->func->
				resource_addr(nvxx_device(device), 1);
			args.limit = args.start + device->info.ram_user - 1;
		} else {
			args.target = NV_DMA_V0_TARGET_VRAM;
			args.access = NV_DMA_V0_ACCESS_RDWR;
			args.start = 0;
			args.limit = device->info.ram_user - 1;
		}
	} else {
		if (chan->drm->agp.stat == ENABLED) {
			args.target = NV_DMA_V0_TARGET_AGP;
			args.access = NV_DMA_V0_ACCESS_RDWR;
			args.start = chan->drm->agp.base;
			args.limit = chan->drm->agp.base +
				     chan->drm->agp.size - 1;
		} else {
			args.target = NV_DMA_V0_TARGET_VM;
			args.access = NV_DMA_V0_ACCESS_RDWR;
			args.start = 0;
			args.limit = mmu->limit - 1;
		}
	}

	ret = nvif_object_init(&device->object, NVDRM_PUSH |
			       (handle & 0xffff), NV_DMA_FROM_MEMORY,
			       &args, sizeof(args), &chan->push.ctxdma);
	if (ret) {
		nouveau_channel_del(pchan);
		return ret;
	}

	return 0;
}
