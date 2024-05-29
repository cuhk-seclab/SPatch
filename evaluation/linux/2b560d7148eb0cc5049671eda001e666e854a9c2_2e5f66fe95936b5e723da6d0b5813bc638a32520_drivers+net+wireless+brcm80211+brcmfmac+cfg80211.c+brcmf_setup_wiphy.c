static int brcmf_setup_wiphy(struct wiphy *wiphy, struct brcmf_if *ifp)
{
	struct ieee80211_supported_band *band;
	__le32 bandlist[3];
	u32 n_bands;
	int err, i;

	wiphy->max_scan_ssids = WL_NUM_SCAN_MAX;
	wiphy->max_scan_ie_len = BRCMF_SCAN_IE_LEN_MAX;
	wiphy->max_num_pmkids = WL_NUM_PMKIDS_MAX;

	err = brcmf_setup_ifmodes(wiphy, ifp);
	if (err)
		return err;

	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	wiphy->cipher_suites = __wl_cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(__wl_cipher_suites);
	wiphy->flags |= WIPHY_FLAG_PS_ON_BY_DEFAULT |
			WIPHY_FLAG_OFFCHAN_TX |
			WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL |
			WIPHY_FLAG_SUPPORTS_TDLS;
	if (!brcmf_roamoff)
		wiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM;
	wiphy->mgmt_stypes = brcmf_txrx_stypes;
	wiphy->max_remain_on_channel_duration = 5000;
	if (brcmf_feat_is_enabled(ifp, BRCMF_FEAT_PNO))
		brcmf_wiphy_pno_params(wiphy);

	/* vendor commands/events support */
	wiphy->vendor_commands = brcmf_vendor_cmds;
	wiphy->n_vendor_commands = BRCMF_VNDR_CMDS_LAST - 1;

	if (brcmf_feat_is_enabled(ifp, BRCMF_FEAT_WOWL))
		brcmf_wiphy_wowl_params(wiphy);

	err = brcmf_fil_cmd_data_get(ifp, BRCMF_C_GET_BANDLIST, &bandlist,
				     sizeof(bandlist));
	if (err) {
		brcmf_err("could not obtain band info: err=%d\n", err);
		return err;
	}
	/* first entry in bandlist is number of bands */
	n_bands = le32_to_cpu(bandlist[0]);
	for (i = 1; i <= n_bands && i < ARRAY_SIZE(bandlist); i++) {
		if (bandlist[i] == cpu_to_le32(WLC_BAND_2G)) {
			band = kmemdup(&__wl_band_2ghz, sizeof(__wl_band_2ghz),
				       GFP_KERNEL);
			if (!band)
				return -ENOMEM;

			band->channels = kmemdup(&__wl_2ghz_channels,
						 sizeof(__wl_2ghz_channels),
						 GFP_KERNEL);
			if (!band->channels) {
				kfree(band);
				return -ENOMEM;
			}

			band->n_channels = ARRAY_SIZE(__wl_2ghz_channels);
			wiphy->bands[IEEE80211_BAND_2GHZ] = band;
		}
		if (bandlist[i] == cpu_to_le32(WLC_BAND_5G)) {
			band = kmemdup(&__wl_band_5ghz, sizeof(__wl_band_5ghz),
				       GFP_KERNEL);
			if (!band)
				return -ENOMEM;

			band->channels = kmemdup(&__wl_5ghz_channels,
						 sizeof(__wl_5ghz_channels),
						 GFP_KERNEL);
			if (!band->channels) {
				kfree(band);
				return -ENOMEM;
			}

			band->n_channels = ARRAY_SIZE(__wl_5ghz_channels);
			wiphy->bands[IEEE80211_BAND_5GHZ] = band;
		}
	}
	err = brcmf_setup_wiphybands(wiphy);
	return err;
}
