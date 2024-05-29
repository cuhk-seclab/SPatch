static void
gf110_disp_intr_unk2_2(struct nv50_disp *disp, int head)
{
	struct nvkm_device *device = disp->base.engine.subdev.device;
	struct nvkm_output *outp;
	u32 pclk = nvkm_rd32(device, 0x660450 + (head * 0x300)) / 1000;
	u32 conf, addr, data;

	outp = exec_clkcmp(disp, head, 0xff, pclk, &conf);
	if (!outp)
		return;

	/* see note in nv50_disp_intr_unk20_2() */
	if (outp->info.type == DCB_OUTPUT_DP) {
		u32 sync = nvkm_rd32(device, 0x660404 + (head * 0x300));
		switch ((sync & 0x000003c0) >> 6) {
		case 6: pclk = pclk * 30; break;
		case 5: pclk = pclk * 24; break;
		case 2:
		default:
			pclk = pclk * 18;
			break;
		}

		if (nvkm_output_dp_train(outp, pclk, true))
			OUTP_ERR(outp, "link not trained before attach");
	} else {
		if (disp->sor.magic)
			disp->sor.magic(outp);
	}

	exec_clkcmp(disp, head, 0, pclk, &conf);

	if (outp->info.type == DCB_OUTPUT_ANALOG) {
		addr = 0x612280 + (ffs(outp->info.or) - 1) * 0x800;
		data = 0x00000000;
	} else {
		addr = 0x612300 + (ffs(outp->info.or) - 1) * 0x800;
		data = (conf & 0x0100) ? 0x00000101 : 0x00000000;
		switch (outp->info.type) {
		case DCB_OUTPUT_TMDS:
			nvkm_mask(device, addr, 0x007c0000, 0x00280000);
			break;
		case DCB_OUTPUT_DP:
			gf110_disp_intr_unk2_2_tu(disp, head, &outp->info);
			break;
		default:
			break;
		}
	}

	nvkm_mask(device, addr, 0x00000707, data);
}
