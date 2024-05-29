void
gm204_sor_magic(struct nvkm_output *outp)
{
	struct nvkm_device *device = outp->disp->engine.subdev.device;
	const u32 soff = outp->or * 0x100;
	const u32 data = outp->or + 1;
	if (outp->info.sorconf.link & 1)
		nvkm_mask(device, 0x612308 + soff, 0x0000001f, 0x00000000 | data);
	if (outp->info.sorconf.link & 2)
		nvkm_mask(device, 0x612388 + soff, 0x0000001f, 0x00000010 | data);
}
