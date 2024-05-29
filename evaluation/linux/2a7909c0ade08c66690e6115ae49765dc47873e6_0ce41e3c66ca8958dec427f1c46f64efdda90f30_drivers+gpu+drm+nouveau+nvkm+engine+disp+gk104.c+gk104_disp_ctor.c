static int
gk104_disp_ctor(struct nvkm_object *parent, struct nvkm_object *engine,
		struct nvkm_oclass *oclass, void *data, u32 size,
		struct nvkm_object **pobject)
{
	struct nvkm_device *device = (void *)parent;
	struct nv50_disp *disp;
	int heads = nvkm_rd32(device, 0x022448);
	int ret;

	ret = nvkm_disp_create(parent, engine, oclass, heads,
			       "PDISP", "display", &disp);
	*pobject = nv_object(disp);
	if (ret)
		return ret;

	disp->base.func = &gk104_disp;

	ret = nvkm_event_init(&gf119_disp_chan_uevent, 1, 17, &disp->uevent);
	if (ret)
		return ret;

	nv_subdev(disp)->intr = gf119_disp_intr;
	INIT_WORK(&disp->supervisor, gf119_disp_intr_supervisor);
	disp->head.nr = heads;
	disp->dac.nr = 3;
	disp->sor.nr = 4;
	disp->dac.power = nv50_dac_power;
	disp->dac.sense = nv50_dac_sense;
	disp->sor.power = nv50_sor_power;
	disp->sor.hda_eld = gf119_hda_eld;
	disp->sor.hdmi = gk104_hdmi_ctrl;
	return 0;
}
