static void
nv50_disp_intr_error(struct nv50_disp *disp, int chid)
{
	struct nvkm_subdev *subdev = &disp->base.engine.subdev;
	struct nvkm_device *device = subdev->device;
	u32 data = nvkm_rd32(device, 0x610084 + (chid * 0x08));
	u32 addr = nvkm_rd32(device, 0x610080 + (chid * 0x08));
	u32 code = (addr & 0x00ff0000) >> 16;
	u32 type = (addr & 0x00007000) >> 12;
	u32 mthd = (addr & 0x00000ffc);
	const struct nvkm_enum *ec, *et;

	et = nvkm_enum_find(nv50_disp_intr_error_type, type);
	ec = nvkm_enum_find(nv50_disp_intr_error_code, code);

	nvkm_error(subdev,
		   "ERROR %d [%s] %02x [%s] chid %d mthd %04x data %08x\n",
		   type, et ? et->name : "", code, ec ? ec->name : "",
		   chid, mthd, data);

	if (chid < ARRAY_SIZE(disp->chan)) {
		switch (mthd) {
		case 0x0080:
			nv50_disp_chan_mthd(disp->chan[chid], NV_DBG_ERROR);
			break;
		default:
			break;
		}
	}

	nvkm_wr32(device, 0x610020, 0x00010000 << chid);
	nvkm_wr32(device, 0x610080 + (chid * 0x08), 0x90000000);
}
