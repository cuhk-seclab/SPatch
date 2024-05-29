static int
nv50_mxm_ctor(struct nvkm_object *parent, struct nvkm_object *engine,
	      struct nvkm_oclass *oclass, void *data, u32 size,
	      struct nvkm_object **pobject)
{
	struct nv50_mxm_priv *priv;
	int ret;

	ret = nvkm_mxm_create(parent, engine, oclass, &priv);
	*pobject = nv_object(priv);
	if (ret)
		return ret;

	if (priv->base.action & MXM_SANITISE_DCB)
		mxm_dcb_sanitise(&priv->base);
	return 0;
}
