static int
g84_disp_ctor(struct nvkm_object *parent, struct nvkm_object *engine,
	      struct nvkm_oclass *oclass, void *data, u32 size,
	      struct nvkm_object **pobject)
{
	struct nv50_disp *disp;
	int ret;

	ret = nvkm_disp_create(parent, engine, oclass, 2, "PDISP",
			       "display", &disp);
	*pobject = nv_object(disp);
	if (ret)
		return ret;

	disp->base.func = &g84_disp;

	ret = nvkm_event_init(&nv50_disp_chan_uevent, 1, 9, &disp->uevent);
	if (ret)
		return ret;

	nv_subdev(disp)->intr = nv50_disp_intr;
	INIT_WORK(&disp->supervisor, nv50_disp_intr_supervisor);
	disp->head.nr = 2;
	disp->dac.nr = 3;
	disp->sor.nr = 2;
	disp->pior.nr = 3;
	disp->dac.power = nv50_dac_power;
	disp->dac.sense = nv50_dac_sense;
	disp->sor.power = nv50_sor_power;
	disp->sor.hdmi = g84_hdmi_ctrl;
	disp->pior.power = nv50_pior_power;
	return 0;
}
