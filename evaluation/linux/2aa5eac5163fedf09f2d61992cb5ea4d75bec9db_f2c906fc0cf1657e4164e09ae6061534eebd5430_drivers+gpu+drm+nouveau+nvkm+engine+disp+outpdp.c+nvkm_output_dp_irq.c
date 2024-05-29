static int
nvkm_output_dp_irq(struct nvkm_notify *notify)
{
	const struct nvkm_i2c_ntfy_rep *line = notify->data;
	struct nvkm_output_dp *outp = container_of(notify, typeof(*outp), irq);
	struct nvkm_connector *conn = outp->base.conn;
	struct nvkm_disp *disp = outp->base.disp;
	struct nvif_notify_conn_rep_v0 rep = {
		.mask = NVIF_NOTIFY_CONN_V0_IRQ,
	};

	OUTP_DBG(&outp->base, "IRQ: %d", line->mask);
	nvkm_output_dp_train(&outp->base, 0, true);

	nvkm_event_send(&disp->hpd, rep.mask, conn->index, &rep, sizeof(rep));
	return NVKM_NOTIFY_DROP;
}
