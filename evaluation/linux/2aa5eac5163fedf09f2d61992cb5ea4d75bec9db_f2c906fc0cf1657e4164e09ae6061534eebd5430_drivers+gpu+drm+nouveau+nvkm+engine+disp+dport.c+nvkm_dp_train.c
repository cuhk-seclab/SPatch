void
nvkm_dp_train(struct work_struct *w)
{
	struct nvkm_output_dp *outp = container_of(w, typeof(*outp), lt.work);
	struct nv50_disp *disp = (void *)outp->base.disp;
	const struct dp_rates *cfg = nvkm_dp_rates;
	struct dp_state _dp = {
		.outp = outp,
	}, *dp = &_dp;
	u32 datarate = 0;
	int ret;

	if (!outp->base.info.location && disp->sor.magic)
		disp->sor.magic(&outp->base);

	/* bring capabilities within encoder limits */
	if (disp->base.engine.subdev.device->chipset < 0xd0)
		outp->dpcd[2] &= ~DPCD_RC02_TPS3_SUPPORTED;
	if ((outp->dpcd[2] & 0x1f) > outp->base.info.dpconf.link_nr) {
		outp->dpcd[2] &= ~DPCD_RC02_MAX_LANE_COUNT;
		outp->dpcd[2] |= outp->base.info.dpconf.link_nr;
	}
	if (outp->dpcd[1] > outp->base.info.dpconf.link_bw)
		outp->dpcd[1] = outp->base.info.dpconf.link_bw;
	dp->pc2 = outp->dpcd[2] & DPCD_RC02_TPS3_SUPPORTED;

	/* restrict link config to the lowest required rate, if requested */
	if (datarate) {
		datarate = (datarate / 8) * 10; /* 8B/10B coding overhead */
		while (cfg[1].rate >= datarate)
			cfg++;
	}
	cfg--;

	/* disable link interrupt handling during link training */
	nvkm_notify_put(&outp->irq);

	/* enable down-spreading and execute pre-train script from vbios */
	dp_link_train_init(dp, outp->dpcd[3] & 0x01);

	while (ret = -EIO, (++cfg)->rate) {
		/* select next configuration supported by encoder and sink */
		while (cfg->nr > (outp->dpcd[2] & DPCD_RC02_MAX_LANE_COUNT) ||
		       cfg->bw > (outp->dpcd[DPCD_RC01_MAX_LINK_RATE]))
			cfg++;
		dp->link_bw = cfg->bw * 27000;
		dp->link_nr = cfg->nr;

		/* program selected link configuration */
		ret = dp_set_link_config(dp);
		if (ret == 0) {
			/* attempt to train the link at this configuration */
			memset(dp->stat, 0x00, sizeof(dp->stat));
			if (!dp_link_train_cr(dp) &&
			    !dp_link_train_eq(dp))
				break;
		} else
		if (ret) {
			/* dp_set_link_config() handled training, or
			 * we failed to communicate with the sink.
			 */
			break;
		}
	}

	/* finish link training and execute post-train script from vbios */
	dp_set_training_pattern(dp, 0);
	if (ret < 0)
		OUTP_ERR(&outp->base, "link training failed");

	dp_link_train_fini(dp);

	/* signal completion and enable link interrupt handling */
	OUTP_DBG(&outp->base, "training complete");
	atomic_set(&outp->lt.done, 1);
	wake_up(&outp->lt.wait);
	nvkm_notify_get(&outp->irq);
}
