int t4_setup_debugfs(struct adapter *adap)
{
	int i;
	u32 size = 0;
	struct dentry *de;

	static struct t4_debugfs_entry t4_debugfs_files[] = {
		{ "cim_la", &cim_la_fops, S_IRUSR, 0 },
		{ "cim_pif_la", &cim_pif_la_fops, S_IRUSR, 0 },
		{ "cim_ma_la", &cim_ma_la_fops, S_IRUSR, 0 },
		{ "cim_qcfg", &cim_qcfg_fops, S_IRUSR, 0 },
		{ "clk", &clk_debugfs_fops, S_IRUSR, 0 },
		{ "devlog", &devlog_fops, S_IRUSR, 0 },
		{ "mbox0", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 0 },
		{ "mbox1", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 1 },
		{ "mbox2", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 2 },
		{ "mbox3", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 3 },
		{ "mbox4", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 4 },
		{ "mbox5", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 5 },
		{ "mbox6", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 6 },
		{ "mbox7", &mbox_debugfs_fops, S_IRUSR | S_IWUSR, 7 },
		{ "l2t", &t4_l2t_fops, S_IRUSR, 0},
		{ "mps_tcam", &mps_tcam_debugfs_fops, S_IRUSR, 0 },
		{ "rss", &rss_debugfs_fops, S_IRUSR, 0 },
		{ "rss_config", &rss_config_debugfs_fops, S_IRUSR, 0 },
		{ "rss_key", &rss_key_debugfs_fops, S_IRUSR, 0 },
		{ "rss_pf_config", &rss_pf_config_debugfs_fops, S_IRUSR, 0 },
		{ "rss_vf_config", &rss_vf_config_debugfs_fops, S_IRUSR, 0 },
		{ "sge_qinfo", &sge_qinfo_debugfs_fops, S_IRUSR, 0 },
		{ "ibq_tp0",  &cim_ibq_fops, S_IRUSR, 0 },
		{ "ibq_tp1",  &cim_ibq_fops, S_IRUSR, 1 },
		{ "ibq_ulp",  &cim_ibq_fops, S_IRUSR, 2 },
		{ "ibq_sge0", &cim_ibq_fops, S_IRUSR, 3 },
		{ "ibq_sge1", &cim_ibq_fops, S_IRUSR, 4 },
		{ "ibq_ncsi", &cim_ibq_fops, S_IRUSR, 5 },
		{ "obq_ulp0", &cim_obq_fops, S_IRUSR, 0 },
		{ "obq_ulp1", &cim_obq_fops, S_IRUSR, 1 },
		{ "obq_ulp2", &cim_obq_fops, S_IRUSR, 2 },
		{ "obq_ulp3", &cim_obq_fops, S_IRUSR, 3 },
		{ "obq_sge",  &cim_obq_fops, S_IRUSR, 4 },
		{ "obq_ncsi", &cim_obq_fops, S_IRUSR, 5 },
		{ "tp_la", &tp_la_fops, S_IRUSR, 0 },
		{ "ulprx_la", &ulprx_la_fops, S_IRUSR, 0 },
		{ "sensors", &sensors_debugfs_fops, S_IRUSR, 0 },
		{ "pm_stats", &pm_stats_debugfs_fops, S_IRUSR, 0 },
		{ "tx_rate", &tx_rate_debugfs_fops, S_IRUSR, 0 },
		{ "cctrl", &cctrl_tbl_debugfs_fops, S_IRUSR, 0 },
#if IS_ENABLED(CONFIG_IPV6)
		{ "clip_tbl", &clip_tbl_debugfs_fops, S_IRUSR, 0 },
#endif
		{ "blocked_fl", &blocked_fl_fops, S_IRUSR | S_IWUSR, 0 },
	};

	/* Debug FS nodes common to all T5 and later adapters.
	 */
	static struct t4_debugfs_entry t5_debugfs_files[] = {
		{ "obq_sge_rx_q0", &cim_obq_fops, S_IRUSR, 6 },
		{ "obq_sge_rx_q1", &cim_obq_fops, S_IRUSR, 7 },
	};

	add_debugfs_files(adap,
			  t4_debugfs_files,
			  ARRAY_SIZE(t4_debugfs_files));
	if (!is_t4(adap->params.chip))
		add_debugfs_files(adap,
				  t5_debugfs_files,
				  ARRAY_SIZE(t5_debugfs_files));

	i = t4_read_reg(adap, MA_TARGET_MEM_ENABLE_A);
	if (i & EDRAM0_ENABLE_F) {
		size = t4_read_reg(adap, MA_EDRAM0_BAR_A);
		add_debugfs_mem(adap, "edc0", MEM_EDC0, EDRAM0_SIZE_G(size));
	}
	if (i & EDRAM1_ENABLE_F) {
		size = t4_read_reg(adap, MA_EDRAM1_BAR_A);
		add_debugfs_mem(adap, "edc1", MEM_EDC1, EDRAM1_SIZE_G(size));
	}
	if (is_t5(adap->params.chip)) {
		if (i & EXT_MEM0_ENABLE_F) {
			size = t4_read_reg(adap, MA_EXT_MEMORY0_BAR_A);
			add_debugfs_mem(adap, "mc0", MEM_MC0,
					EXT_MEM0_SIZE_G(size));
		}
		if (i & EXT_MEM1_ENABLE_F) {
			size = t4_read_reg(adap, MA_EXT_MEMORY1_BAR_A);
			add_debugfs_mem(adap, "mc1", MEM_MC1,
					EXT_MEM1_SIZE_G(size));
		}
	} else {
		if (i & EXT_MEM_ENABLE_F)
			size = t4_read_reg(adap, MA_EXT_MEMORY_BAR_A);
			add_debugfs_mem(adap, "mc", MEM_MC,
					EXT_MEM_SIZE_G(size));
	}

	de = debugfs_create_file_size("flash", S_IRUSR, adap->debugfs_root, adap,
				      &flash_debugfs_fops, adap->params.sf_size);
	debugfs_create_bool("use_backdoor", S_IWUSR | S_IRUSR,
			    adap->debugfs_root, &adap->use_bd);

	return 0;
}
