static void bnx2x_calc_vn_max(struct bnx2x *bp, int vn,
				    struct cmng_init_input *input)
{
	u16 vn_max_rate;
	u32 vn_cfg = bp->mf_config[vn];

	if (vn_cfg & FUNC_MF_CFG_FUNC_HIDE)
		vn_max_rate = 0;
	else {
		u32 maxCfg = bnx2x_extract_max_cfg(bp, vn_cfg);

		if (IS_MF_PERCENT_BW(bp)) {
			/* maxCfg in percents of linkspeed */
			vn_max_rate = (bp->link_vars.line_speed * maxCfg) / 100;
		} else /* SD modes */
			/* maxCfg is absolute in 100Mb units */
			vn_max_rate = maxCfg * 100;
	}

	DP(NETIF_MSG_IFUP, "vn %d: vn_max_rate %d\n", vn, vn_max_rate);

	input->vnic_max_rate[vn] = vn_max_rate;
}
