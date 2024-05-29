int
nv20_gr_init(struct nvkm_gr *base)
{
	struct nv20_gr *gr = nv20_gr(base);
	struct nvkm_device *device = gr->base.engine.subdev.device;
	u32 tmp, vramsz;
	int i;

	nvkm_wr32(device, NV20_PGRAPH_CHANNEL_CTX_TABLE,
			  nvkm_memory_addr(gr->ctxtab) >> 4);

	if (device->chipset == 0x20) {
		nvkm_wr32(device, NV10_PGRAPH_RDI_INDEX, 0x003d0000);
		for (i = 0; i < 15; i++)
			nvkm_wr32(device, NV10_PGRAPH_RDI_DATA, 0x00000000);
		nvkm_msec(device, 2000,
			if (!nvkm_rd32(device, 0x400700))
				break;
		);
	} else {
		nvkm_wr32(device, NV10_PGRAPH_RDI_INDEX, 0x02c80000);
		for (i = 0; i < 32; i++)
			nvkm_wr32(device, NV10_PGRAPH_RDI_DATA, 0x00000000);
		nvkm_msec(device, 2000,
			if (!nvkm_rd32(device, 0x400700))
				break;
		);
	}

	nvkm_wr32(device, NV03_PGRAPH_INTR   , 0xFFFFFFFF);
	nvkm_wr32(device, NV03_PGRAPH_INTR_EN, 0xFFFFFFFF);

	nvkm_wr32(device, NV04_PGRAPH_DEBUG_0, 0xFFFFFFFF);
	nvkm_wr32(device, NV04_PGRAPH_DEBUG_0, 0x00000000);
	nvkm_wr32(device, NV04_PGRAPH_DEBUG_1, 0x00118700);
	nvkm_wr32(device, NV04_PGRAPH_DEBUG_3, 0xF3CE0475); /* 0x4 = auto ctx switch */
	nvkm_wr32(device, NV10_PGRAPH_DEBUG_4, 0x00000000);
	nvkm_wr32(device, 0x40009C           , 0x00000040);

	if (device->chipset >= 0x25) {
		nvkm_wr32(device, 0x400890, 0x00a8cfff);
		nvkm_wr32(device, 0x400610, 0x304B1FB6);
		nvkm_wr32(device, 0x400B80, 0x1cbd3883);
		nvkm_wr32(device, 0x400B84, 0x44000000);
		nvkm_wr32(device, 0x400098, 0x40000080);
		nvkm_wr32(device, 0x400B88, 0x000000ff);

	} else {
		nvkm_wr32(device, 0x400880, 0x0008c7df);
		nvkm_wr32(device, 0x400094, 0x00000005);
		nvkm_wr32(device, 0x400B80, 0x45eae20e);
		nvkm_wr32(device, 0x400B84, 0x24000000);
		nvkm_wr32(device, 0x400098, 0x00000040);
		nvkm_wr32(device, NV10_PGRAPH_RDI_INDEX, 0x00E00038);
		nvkm_wr32(device, NV10_PGRAPH_RDI_DATA , 0x00000030);
		nvkm_wr32(device, NV10_PGRAPH_RDI_INDEX, 0x00E10038);
		nvkm_wr32(device, NV10_PGRAPH_RDI_DATA , 0x00000030);
	}

	nvkm_wr32(device, 0x4009a0, nvkm_rd32(device, 0x100324));
	nvkm_wr32(device, NV10_PGRAPH_RDI_INDEX, 0x00EA000C);
	nvkm_wr32(device, NV10_PGRAPH_RDI_DATA, nvkm_rd32(device, 0x100324));

	nvkm_wr32(device, NV10_PGRAPH_CTX_CONTROL, 0x10000100);
	nvkm_wr32(device, NV10_PGRAPH_STATE      , 0xFFFFFFFF);

	tmp = nvkm_rd32(device, NV10_PGRAPH_SURFACE) & 0x0007ff00;
	nvkm_wr32(device, NV10_PGRAPH_SURFACE, tmp);
	tmp = nvkm_rd32(device, NV10_PGRAPH_SURFACE) | 0x00020100;
	nvkm_wr32(device, NV10_PGRAPH_SURFACE, tmp);

	/* begin RAM config */
	vramsz = device->func->resource_size(device, 1) - 1;
	nvkm_wr32(device, 0x4009A4, nvkm_rd32(device, 0x100200));
	nvkm_wr32(device, 0x4009A8, nvkm_rd32(device, 0x100204));
	nvkm_wr32(device, NV10_PGRAPH_RDI_INDEX, 0x00EA0000);
	nvkm_wr32(device, NV10_PGRAPH_RDI_DATA , nvkm_rd32(device, 0x100200));
	nvkm_wr32(device, NV10_PGRAPH_RDI_INDEX, 0x00EA0004);
	nvkm_wr32(device, NV10_PGRAPH_RDI_DATA , nvkm_rd32(device, 0x100204));
	nvkm_wr32(device, 0x400820, 0);
	nvkm_wr32(device, 0x400824, 0);
	nvkm_wr32(device, 0x400864, vramsz - 1);
	nvkm_wr32(device, 0x400868, vramsz - 1);

	/* interesting.. the below overwrites some of the tile setup above.. */
	nvkm_wr32(device, 0x400B20, 0x00000000);
	nvkm_wr32(device, 0x400B04, 0xFFFFFFFF);

	nvkm_wr32(device, NV03_PGRAPH_ABS_UCLIP_XMIN, 0);
	nvkm_wr32(device, NV03_PGRAPH_ABS_UCLIP_YMIN, 0);
	nvkm_wr32(device, NV03_PGRAPH_ABS_UCLIP_XMAX, 0x7fff);
	nvkm_wr32(device, NV03_PGRAPH_ABS_UCLIP_YMAX, 0x7fff);
	return 0;
}
