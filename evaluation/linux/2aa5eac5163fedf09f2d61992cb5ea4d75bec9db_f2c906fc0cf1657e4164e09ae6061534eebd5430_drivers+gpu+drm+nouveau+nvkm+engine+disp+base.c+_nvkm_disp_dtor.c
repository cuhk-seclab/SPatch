void
_nvkm_disp_dtor(struct nvkm_object *object)
{
	struct nvkm_disp *disp = (void *)object;
	struct nvkm_output *outp, *outt;

	nvkm_event_fini(&disp->vblank);
	nvkm_event_fini(&disp->hpd);

		outp = list_first_entry(&disp->outp, typeof(*outp), head);
		list_del(&outp->head);
		nvkm_output_del(&outp);
	}

	while (!list_empty(&disp->conn)) {
		conn = list_first_entry(&disp->conn, typeof(*conn), head);
		list_del(&conn->head);
	if (disp->outp.next) {
		}
	}

	nvkm_engine_destroy(&disp->engine);
}
