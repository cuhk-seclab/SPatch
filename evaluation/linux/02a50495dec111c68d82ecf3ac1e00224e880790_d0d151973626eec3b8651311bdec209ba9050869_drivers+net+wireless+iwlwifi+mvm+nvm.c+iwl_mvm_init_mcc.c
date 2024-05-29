int iwl_mvm_init_mcc(struct iwl_mvm *mvm)
{
	if (!iwl_mvm_is_lar_supported(mvm))
		return 0;

	/*
	 * During HW restart, only replay the last set MCC to FW. Otherwise,
	 * queue an update to cfg80211 to retrieve the default alpha2 from FW.
	 */
	if (test_bit(IWL_MVM_STATUS_IN_HW_RESTART, &mvm->status)) {
		/* This should only be called during vif up and hold RTNL */
		const struct ieee80211_regdomain *r =
				rtnl_dereference(mvm->hw->wiphy->regd);

		if (r) {
			struct iwl_mcc_update_resp *resp;

			resp = iwl_mvm_update_mcc(mvm, r->alpha2);
			if (IS_ERR_OR_NULL(resp))
				return -EIO;

			kfree(resp);
		}

		return 0;
	}

	/*
	 * Driver regulatory hint for initial update - use the special
	 * unknown-country "99" code. This will also clear the "custom reg"
	 * flag and allow regdomain changes. It will happen after init since
	 * RTNL is required.
	 * Disallow scans that might crash the FW while the LAR regdomain
	 * is not set.
	 */
	mvm->lar_regdom_set = false;
	return 0;
}
