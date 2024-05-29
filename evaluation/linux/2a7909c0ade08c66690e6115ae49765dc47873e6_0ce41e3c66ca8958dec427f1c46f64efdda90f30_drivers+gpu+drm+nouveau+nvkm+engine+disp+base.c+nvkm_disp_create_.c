int
nvkm_disp_create_(struct nvkm_object *parent, struct nvkm_object *engine,
		  struct nvkm_oclass *oclass, int heads, const char *intname,
		  const char *extname, int length, void **pobject)
{
	struct nvkm_disp_impl *impl = (void *)oclass;
	struct nvkm_device *device = (void *)parent;
	struct nvkm_bios *bios = device->bios;
	struct nvkm_disp *disp;
	struct nvkm_connector *conn;
	struct nvkm_output *outp, *outt, *pair;
	struct nvbios_connE connE;
	struct dcb_output dcbE;
	u8  hpd = 0, ver, hdr;
	u32 data;
	int ret, i;

	ret = nvkm_engine_create_(parent, engine, oclass, true, intname,
				  extname, length, pobject);
	disp = *pobject;
	if (ret)
		return ret;

	disp->engine.func = &nvkm_disp;
	INIT_LIST_HEAD(&disp->outp);
	INIT_LIST_HEAD(&disp->conn);

	/* create output objects for each display path in the vbios */
	i = -1;
	while ((data = dcb_outp_parse(bios, ++i, &ver, &hdr, &dcbE))) {
		const struct nvkm_disp_func_outp *outps;
		int (*ctor)(struct nvkm_disp *, int, struct dcb_output *,
			    struct nvkm_output **);

		if (dcbE.type == DCB_OUTPUT_UNUSED)
			continue;
		if (dcbE.type == DCB_OUTPUT_EOL)
			break;
		outp = NULL;

		switch (dcbE.location) {
		case 0: outps = &impl->outp.internal; break;
		case 1: outps = &impl->outp.external; break;
		default:
			nvkm_warn(&disp->engine.subdev,
				  "dcb %d locn %d unknown\n", i, dcbE.location);
			continue;
		}

		switch (dcbE.type) {
		case DCB_OUTPUT_ANALOG: ctor = outps->crt ; break;
		case DCB_OUTPUT_TV    : ctor = outps->tv  ; break;
		case DCB_OUTPUT_TMDS  : ctor = outps->tmds; break;
		case DCB_OUTPUT_LVDS  : ctor = outps->lvds; break;
		case DCB_OUTPUT_DP    : ctor = outps->dp  ; break;
		default:
			nvkm_warn(&disp->engine.subdev,
				  "dcb %d type %d unknown\n", i, dcbE.type);
			continue;
		}

		if (ctor)
			ret = ctor(disp, i, &dcbE, &outp);
		else
			ret = -ENODEV;

		if (ret) {
			if (ret == -ENODEV) {
				nvkm_debug(&disp->engine.subdev,
					   "dcb %d %d/%d not supported\n",
					   i, dcbE.location, dcbE.type);
				continue;
			}
			nvkm_error(&disp->engine.subdev,
				   "failed to create output %d\n", i);
			nvkm_output_del(&outp);
			continue;
		}

		list_add_tail(&outp->head, &disp->outp);
		hpd = max(hpd, (u8)(dcbE.connector + 1));
	}

	/* create connector objects based on the outputs we support */
	list_for_each_entry_safe(outp, outt, &disp->outp, head) {
		/* bios data *should* give us the most useful information */
		data = nvbios_connEp(bios, outp->info.connector, &ver, &hdr,
				     &connE);

		/* no bios connector data... */
		if (!data) {
			/* heuristic: anything with the same ccb index is
			 * considered to be on the same connector, any
			 * output path without an associated ccb entry will
			 * be put on its own connector
			 */
			int ccb_index = outp->info.i2c_index;
			if (ccb_index != 0xf) {
				list_for_each_entry(pair, &disp->outp, head) {
					if (pair->info.i2c_index == ccb_index) {
						outp->conn = pair->conn;
						break;
					}
				}
			}

			/* connector shared with another output path */
			if (outp->conn)
				continue;

			memset(&connE, 0x00, sizeof(connE));
			connE.type = DCB_CONNECTOR_NONE;
			i = -1;
		} else {
			i = outp->info.connector;
		}

		/* check that we haven't already created this connector */
		list_for_each_entry(conn, &disp->conn, head) {
			if (conn->index == outp->info.connector) {
				outp->conn = conn;
				break;
			}
		}

		if (outp->conn)
			continue;

		/* apparently we need to create a new one! */
		ret = nvkm_connector_new(disp, i, &connE, &outp->conn);
		if (ret) {
			nvkm_error(&disp->engine.subdev,
				   "failed to create output %d conn: %d\n",
				   outp->index, ret);
			nvkm_connector_del(&outp->conn);
			list_del(&outp->head);
			nvkm_output_del(&outp);
			continue;
		}

		list_add_tail(&outp->conn->head, &disp->conn);
	}

	ret = nvkm_event_init(&nvkm_disp_hpd_func, 3, hpd, &disp->hpd);
	if (ret)
		return ret;

	ret = nvkm_event_init(impl->vblank, 1, heads, &disp->vblank);
	if (ret)
		return ret;

	return 0;
}
