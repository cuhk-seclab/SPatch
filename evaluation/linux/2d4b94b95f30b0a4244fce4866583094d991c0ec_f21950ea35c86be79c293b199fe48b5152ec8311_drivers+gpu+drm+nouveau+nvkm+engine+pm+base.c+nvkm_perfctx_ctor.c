static int
nvkm_perfctx_ctor(struct nvkm_object *parent, struct nvkm_object *engine,
		  struct nvkm_oclass *oclass, void *data, u32 size,
		  struct nvkm_object **pobject)
{
	struct nvkm_pm *ppm = (void *)engine;
	struct nvkm_perfctx *ctx;
	int ret;

	/* no context needed for perfdom objects... */
	if (nv_mclass(parent) != NV_DEVICE) {
		atomic_inc(&parent->refcount);
		*pobject = parent;
		return 1;
	}

	ret = nvkm_engctx_create(parent, engine, oclass, NULL, 0, 0, 0, &ctx);
	*pobject = nv_object(ctx);
	if (ret)
		return ret;

	mutex_lock(&nv_subdev(ppm)->mutex);
	if (ppm->context == NULL)
		ppm->context = ctx;
	if (ctx != ppm->context)
		ret = -EBUSY;
	mutex_unlock(&nv_subdev(ppm)->mutex);

	return ret;
}
