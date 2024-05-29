int
gf110_sor_dp_lnk_ctl(struct nvkm_output_dp *outp, int nr, int bw, bool ef)
{
	struct nvkm_device *device = outp->base.disp->engine.subdev.device;
	const u32 soff = gf110_sor_soff(outp);
	const u32 loff = gf110_sor_loff(outp);
	u32 dpctrl = 0x00000000;
	u32 clksor = 0x00000000;

	clksor |= bw << 18;
	dpctrl |= ((1 << nr) - 1) << 16;
	if (ef)
		dpctrl |= 0x00004000;

	nvkm_mask(device, 0x612300 + soff, 0x007c0000, clksor);
	nvkm_mask(device, 0x61c10c + loff, 0x001f4000, dpctrl);
	return 0;
}
