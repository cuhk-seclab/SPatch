void iwl_mvm_bt_rssi_event_old(struct iwl_mvm *mvm, struct ieee80211_vif *vif,
			       enum ieee80211_rssi_event rssi_event)
{
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);
	struct iwl_bt_iterator_data data = {
		.mvm = mvm,
	};
	int ret;

	lockdep_assert_held(&mvm->mutex);

	/* Ignore updates if we are in force mode */
	if (unlikely(mvm->bt_force_ant_mode != BT_FORCE_ANT_DIS))
		return;

	/*
	 * Rssi update while not associated - can happen since the statistics
	 * are handled asynchronously
	 */
	if (mvmvif->ap_sta_id == IWL_MVM_STATION_COUNT)
		return;

	/* No BT - reports should be disabled */
	if (!mvm->last_bt_notif_old.bt_status)
		return;

	IWL_DEBUG_COEX(mvm, "RSSI for %pM is now %s\n", vif->bss_conf.bssid,
		       rssi_event == RSSI_EVENT_HIGH ? "HIGH" : "LOW");

	/*
	 * Check if rssi is good enough for reduced Tx power, but not in loose
	 * scheme.
	 */
	if (rssi_event == RSSI_EVENT_LOW || mvm->cfg->bt_shared_single_ant ||
	    iwl_get_coex_type(mvm, vif) == BT_COEX_LOOSE_LUT)
		ret = iwl_mvm_bt_coex_reduced_txp(mvm, mvmvif->ap_sta_id,
						  false);
	else
		ret = iwl_mvm_bt_coex_reduced_txp(mvm, mvmvif->ap_sta_id, true);

	if (ret)
		IWL_ERR(mvm, "couldn't send BT_CONFIG HCMD upon RSSI event\n");

	ieee80211_iterate_active_interfaces_atomic(
		mvm->hw, IEEE80211_IFACE_ITER_NORMAL,
		iwl_mvm_bt_rssi_iterator, &data);

	if (iwl_mvm_bt_udpate_ctrl_kill_msk(mvm))
		IWL_ERR(mvm, "Failed to update the ctrl_kill_msk\n");
}
