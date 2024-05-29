static u32 iwl_mvm_scan_umac_flags(struct iwl_mvm *mvm,
				   struct iwl_mvm_scan_params *params)
{
	int flags = 0;

	if (params->n_ssids == 0)
		flags = IWL_UMAC_SCAN_GEN_FLAGS_PASSIVE;

	if (params->n_ssids == 1 && params->ssids[0].ssid_len != 0)
		flags |= IWL_UMAC_SCAN_GEN_FLAGS_PRE_CONNECT;

	if (params->passive_fragmented)
		flags |= IWL_UMAC_SCAN_GEN_FLAGS_FRAGMENTED;

	if (iwl_mvm_rrm_scan_needed(mvm))
		flags |= IWL_UMAC_SCAN_GEN_FLAGS_RRM_ENABLED;

	if (params->pass_all)
		flags |= IWL_UMAC_SCAN_GEN_FLAGS_PASS_ALL;
	else
		flags |= IWL_UMAC_SCAN_GEN_FLAGS_MATCH;

	if (iwl_mvm_scan_total_iterations(params) > 1)
		flags |= IWL_UMAC_SCAN_GEN_FLAGS_PERIODIC;

#ifdef CONFIG_IWLWIFI_DEBUGFS
	if (mvm->scan_iter_notif_enabled)
		flags |= IWL_UMAC_SCAN_GEN_FLAGS_ITER_COMPLETE;
#endif
	return flags;
}
