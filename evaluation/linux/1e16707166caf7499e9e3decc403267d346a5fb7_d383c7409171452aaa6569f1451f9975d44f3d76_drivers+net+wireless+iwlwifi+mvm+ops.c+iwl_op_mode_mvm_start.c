static struct iwl_op_mode *
iwl_op_mode_mvm_start(struct iwl_trans *trans, const struct iwl_cfg *cfg,
		      const struct iwl_fw *fw, struct dentry *dbgfs_dir)
{
	struct ieee80211_hw *hw;
	struct iwl_op_mode *op_mode;
	struct iwl_mvm *mvm;
	struct iwl_trans_config trans_cfg = {};
	static const u8 no_reclaim_cmds[] = {
		TX_CMD,
	};
	int err, scan_size;
	u32 min_backoff;

	/*
	 * We use IWL_MVM_STATION_COUNT to check the validity of the station
	 * index all over the driver - check that its value corresponds to the
	 * array size.
	 */
	BUILD_BUG_ON(ARRAY_SIZE(mvm->fw_id_to_mac_id) != IWL_MVM_STATION_COUNT);

	/********************************
	 * 1. Allocating and configuring HW data
	 ********************************/
	hw = ieee80211_alloc_hw(sizeof(struct iwl_op_mode) +
				sizeof(struct iwl_mvm),
				&iwl_mvm_hw_ops);
	if (!hw)
		return NULL;

	if (cfg->max_rx_agg_size)
		hw->max_rx_aggregation_subframes = cfg->max_rx_agg_size;

	if (cfg->max_tx_agg_size)
		hw->max_tx_aggregation_subframes = cfg->max_tx_agg_size;

	op_mode = hw->priv;
	op_mode->ops = &iwl_mvm_ops;

	mvm = IWL_OP_MODE_GET_MVM(op_mode);
	mvm->dev = trans->dev;
	mvm->trans = trans;
	mvm->cfg = cfg;
	mvm->fw = fw;
	mvm->hw = hw;

	mvm->restart_fw = iwlwifi_mod_params.restart_fw ? -1 : 0;

	mvm->aux_queue = 15;
	mvm->first_agg_queue = 16;
	mvm->last_agg_queue = mvm->cfg->base_params->num_of_queues - 1;
	if (mvm->cfg->base_params->num_of_queues == 16) {
		mvm->aux_queue = 11;
		mvm->first_agg_queue = 12;
	}
	mvm->sf_state = SF_UNINIT;
	mvm->low_latency_agg_frame_limit = 6;
	mvm->cur_ucode = IWL_UCODE_INIT;

	mutex_init(&mvm->mutex);
	mutex_init(&mvm->d0i3_suspend_mutex);
	spin_lock_init(&mvm->async_handlers_lock);
	INIT_LIST_HEAD(&mvm->time_event_list);
	INIT_LIST_HEAD(&mvm->aux_roc_te_list);
	INIT_LIST_HEAD(&mvm->async_handlers_list);
	spin_lock_init(&mvm->time_event_lock);

	INIT_WORK(&mvm->async_handlers_wk, iwl_mvm_async_handlers_wk);
	INIT_WORK(&mvm->roc_done_wk, iwl_mvm_roc_done_wk);
	INIT_WORK(&mvm->sta_drained_wk, iwl_mvm_sta_drained_wk);
	INIT_WORK(&mvm->d0i3_exit_work, iwl_mvm_d0i3_exit_work);
	INIT_DELAYED_WORK(&mvm->fw_dump_wk, iwl_mvm_fw_error_dump_wk);
	INIT_DELAYED_WORK(&mvm->tdls_cs.dwork, iwl_mvm_tdls_ch_switch_work);

	spin_lock_init(&mvm->d0i3_tx_lock);
	spin_lock_init(&mvm->refs_lock);
	skb_queue_head_init(&mvm->d0i3_tx);
	init_waitqueue_head(&mvm->d0i3_exit_waitq);

	SET_IEEE80211_DEV(mvm->hw, mvm->trans->dev);

	/*
	 * Populate the state variables that the transport layer needs
	 * to know about.
	 */
	trans_cfg.op_mode = op_mode;
	trans_cfg.no_reclaim_cmds = no_reclaim_cmds;
	trans_cfg.n_no_reclaim_cmds = ARRAY_SIZE(no_reclaim_cmds);
	trans_cfg.rx_buf_size_8k = iwlwifi_mod_params.amsdu_size_8K;

	if (mvm->fw->ucode_capa.flags & IWL_UCODE_TLV_FLAGS_DW_BC_TABLE)
		trans_cfg.bc_table_dword = true;

	trans_cfg.command_names = iwl_mvm_cmd_strings;

	trans_cfg.cmd_queue = IWL_MVM_CMD_QUEUE;
	trans_cfg.cmd_fifo = IWL_MVM_TX_FIFO_CMD;
	trans_cfg.scd_set_active = true;

	trans_cfg.sdio_adma_addr = fw->sdio_adma_addr;

	/* Set a short watchdog for the command queue */
	trans_cfg.cmd_q_wdg_timeout =
		iwl_mvm_get_wd_timeout(mvm, NULL, false, true);

	snprintf(mvm->hw->wiphy->fw_version,
		 sizeof(mvm->hw->wiphy->fw_version),
		 "%s", fw->fw_version);

	/* Configure transport layer */
	iwl_trans_configure(mvm->trans, &trans_cfg);

	trans->rx_mpdu_cmd = REPLY_RX_MPDU_CMD;
	trans->rx_mpdu_cmd_hdr_size = sizeof(struct iwl_rx_mpdu_res_start);
	trans->dbg_dest_tlv = mvm->fw->dbg_dest_tlv;
	trans->dbg_dest_reg_num = mvm->fw->dbg_dest_reg_num;
	memcpy(trans->dbg_conf_tlv, mvm->fw->dbg_conf_tlv,
	       sizeof(trans->dbg_conf_tlv));
	trans->dbg_trigger_tlv = mvm->fw->dbg_trigger_tlv;

	/* set up notification wait support */
	iwl_notification_wait_init(&mvm->notif_wait);

	/* Init phy db */
	mvm->phy_db = iwl_phy_db_init(trans);
	if (!mvm->phy_db) {
		IWL_ERR(mvm, "Cannot init phy_db\n");
		goto out_free;
	}

	IWL_INFO(mvm, "Detected %s, REV=0x%X\n",
		 mvm->cfg->name, mvm->trans->hw_rev);

	min_backoff = calc_min_backoff(trans, cfg);
	iwl_mvm_tt_initialize(mvm, min_backoff);
	/* set the nvm_file_name according to priority */
	if (iwlwifi_mod_params.nvm_file)
		mvm->nvm_file_name = iwlwifi_mod_params.nvm_file;
	else
		mvm->nvm_file_name = mvm->cfg->default_nvm_file;

	if (WARN(cfg->no_power_up_nic_in_init && !mvm->nvm_file_name,
		 "not allowing power-up and not having nvm_file\n"))
		goto out_free;

	/*
	 * Even if nvm exists in the nvm_file driver should read again the nvm
	 * from the nic because there might be entries that exist in the OTP
	 * and not in the file.
	 * for nics with no_power_up_nic_in_init: rely completley on nvm_file
	 */
	if (cfg->no_power_up_nic_in_init && mvm->nvm_file_name) {
		err = iwl_nvm_init(mvm, false);
		if (err)
			goto out_free;
	} else {
		err = iwl_trans_start_hw(mvm->trans);
		if (err)
			goto out_free;

		mutex_lock(&mvm->mutex);
		err = iwl_run_init_mvm_ucode(mvm, true);
		if (!err || !iwlmvm_mod_params.init_dbg)
			iwl_trans_stop_device(trans);
		mutex_unlock(&mvm->mutex);
		/* returns 0 if successful, 1 if success but in rfkill */
		if (err < 0 && !iwlmvm_mod_params.init_dbg) {
			IWL_ERR(mvm, "Failed to run INIT ucode: %d\n", err);
			goto out_free;
		}
	}

	scan_size = iwl_mvm_scan_size(mvm);

	mvm->scan_cmd = kmalloc(scan_size, GFP_KERNEL);
	if (!mvm->scan_cmd)
		goto out_free;

	/* Set EBS as successful as long as not stated otherwise by the FW. */
	mvm->last_ebs_successful = true;

	err = iwl_mvm_mac_setup_register(mvm);
	if (err)
		goto out_free;

	err = iwl_mvm_dbgfs_register(mvm, dbgfs_dir);
	if (err)
		goto out_unregister;

	memset(&mvm->rx_stats, 0, sizeof(struct mvm_statistics_rx));

	/* rpm starts with a taken ref. only set the appropriate bit here. */
	mvm->refs[IWL_MVM_REF_UCODE_DOWN] = 1;

	return op_mode;

 out_unregister:
	ieee80211_unregister_hw(mvm->hw);
	iwl_mvm_leds_exit(mvm);
 out_free:
	iwl_phy_db_free(mvm->phy_db);
	kfree(mvm->scan_cmd);
	if (!cfg->no_power_up_nic_in_init || !mvm->nvm_file_name)
		iwl_trans_op_mode_leave(trans);
	ieee80211_free_hw(mvm->hw);
	return NULL;
}
