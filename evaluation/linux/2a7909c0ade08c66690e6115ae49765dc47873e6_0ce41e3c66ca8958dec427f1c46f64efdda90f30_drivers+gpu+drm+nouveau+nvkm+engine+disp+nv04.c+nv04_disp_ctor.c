static int
nv04_disp_ctor(struct nvkm_object *parent, struct nvkm_object *engine,
	       struct nvkm_oclass *oclass, void *data, u32 size,
	       struct nvkm_object **pobject)
{
	struct nvkm_disp *disp;
	int ret;

	ret = nvkm_disp_create(parent, engine, oclass, 2, "DISPLAY",
			       "display", &disp);
	*pobject = nv_object(disp);
	if (ret)
		return ret;

	disp->func = &nv04_disp;

	nv_subdev(disp)->intr = nv04_disp_intr;
	return 0;
}
