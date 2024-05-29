int
nv50_disp_main_mthd(struct nvkm_object *object, u32 mthd, void *data, u32 size)
{
	const struct nv50_disp_impl *impl = (void *)nv_oclass(object->engine);
	union {
		struct nv50_disp_mthd_v0 v0;
		struct nv50_disp_mthd_v1 v1;
	} *args = data;
	struct nv50_disp *disp = (void *)object->engine;
	struct nvkm_output *outp = NULL;
	struct nvkm_output *temp;
	u16 type, mask = 0;
	int head, ret;

	if (mthd != NV50_DISP_MTHD)
		return -EINVAL;

	nvif_ioctl(object, "disp mthd size %d\n", size);
	if (nvif_unpack(args->v0, 0, 0, true)) {
		nvif_ioctl(object, "disp mthd vers %d mthd %02x head %d\n",
			   args->v0.version, args->v0.method, args->v0.head);
		mthd = args->v0.method;
		head = args->v0.head;
	} else
	if (nvif_unpack(args->v1, 1, 1, true)) {
		nvif_ioctl(object, "disp mthd vers %d mthd %02x "
				   "type %04x mask %04x\n",
			   args->v1.version, args->v1.method,
			   args->v1.hasht, args->v1.hashm);
		mthd = args->v1.method;
		type = args->v1.hasht;
		mask = args->v1.hashm;
		head = ffs((mask >> 8) & 0x0f) - 1;
	} else
		return ret;

	if (head < 0 || head >= disp->head.nr)
		return -ENXIO;

	if (mask) {
		list_for_each_entry(temp, &disp->base.outp, head) {
			if ((temp->info.hasht         == type) &&
			    (temp->info.hashm & mask) == mask) {
				outp = temp;
				break;
			}
		}
		if (outp == NULL)
			return -ENXIO;
	}

	switch (mthd) {
	case NV50_DISP_SCANOUTPOS:
		return impl->head.scanoutpos(object, disp, data, size, head);
	default:
		break;
	}

	switch (mthd * !!outp) {
	case NV50_DISP_MTHD_V1_DAC_PWR:
		return disp->dac.power(object, disp, data, size, head, outp);
	case NV50_DISP_MTHD_V1_DAC_LOAD:
		return disp->dac.sense(object, disp, data, size, head, outp);
	case NV50_DISP_MTHD_V1_SOR_PWR:
		return disp->sor.power(object, disp, data, size, head, outp);
	case NV50_DISP_MTHD_V1_SOR_HDA_ELD:
		if (!disp->sor.hda_eld)
			return -ENODEV;
		return disp->sor.hda_eld(object, disp, data, size, head, outp);
	case NV50_DISP_MTHD_V1_SOR_HDMI_PWR:
		if (!disp->sor.hdmi)
			return -ENODEV;
		return disp->sor.hdmi(object, disp, data, size, head, outp);
	case NV50_DISP_MTHD_V1_SOR_LVDS_SCRIPT: {
		union {
			struct nv50_disp_sor_lvds_script_v0 v0;
		} *args = data;
		nvif_ioctl(object, "disp sor lvds script size %d\n", size);
		if (nvif_unpack(args->v0, 0, 0, false)) {
			nvif_ioctl(object, "disp sor lvds script "
					   "vers %d name %04x\n",
				   args->v0.version, args->v0.script);
			disp->sor.lvdsconf = args->v0.script;
			return 0;
		} else
			return ret;
	}
		break;
	case NV50_DISP_MTHD_V1_SOR_DP_PWR: {
		struct nvkm_output_dp *outpdp = nvkm_output_dp(outp);
		union {
			struct nv50_disp_sor_dp_pwr_v0 v0;
		} *args = data;
		nvif_ioctl(object, "disp sor dp pwr size %d\n", size);
		if (nvif_unpack(args->v0, 0, 0, false)) {
			nvif_ioctl(object, "disp sor dp pwr vers %d state %d\n",
				   args->v0.version, args->v0.state);
			if (args->v0.state == 0) {
				nvkm_notify_put(&outpdp->irq);
				outpdp->func->lnk_pwr(outpdp, 0);
				atomic_set(&outpdp->lt.done, 0);
				return 0;
			} else
			if (args->v0.state != 0) {
				nvkm_output_dp_train(&outpdp->base, 0, true);
				return 0;
			}
		} else
			return ret;
	}
		break;
	case NV50_DISP_MTHD_V1_PIOR_PWR:
		if (!disp->pior.power)
			return -ENODEV;
		return disp->pior.power(object, disp, data, size, head, outp);
	default:
		break;
	}

	return -EINVAL;
}
