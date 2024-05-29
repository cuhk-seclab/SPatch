struct iwl_nvm_data *
iwl_parse_nvm_data(struct device *dev, const struct iwl_cfg *cfg,
		   const __le16 *nvm_hw, const __le16 *nvm_sw,
		   const __le16 *nvm_calib, const __le16 *regulatory,
		   const __le16 *mac_override, u8 tx_chains, u8 rx_chains,
		   bool lar_supported)
{
	struct iwl_nvm_data *data;
	u32 sku;
	u32 radio_cfg;

	if (cfg->device_family != IWL_DEVICE_FAMILY_8000)
		data = kzalloc(sizeof(*data) +
			       sizeof(struct ieee80211_channel) *
			       IWL_NUM_CHANNELS,
			       GFP_KERNEL);
	else
		data = kzalloc(sizeof(*data) +
			       sizeof(struct ieee80211_channel) *
			       IWL_NUM_CHANNELS_FAMILY_8000,
			       GFP_KERNEL);
	if (!data)
		return NULL;

	data->nvm_version = iwl_get_nvm_version(cfg, nvm_sw);

	radio_cfg = iwl_get_radio_cfg(cfg, nvm_sw);
	iwl_set_radio_cfg(cfg, data, radio_cfg);
	if (data->valid_tx_ant)
		tx_chains &= data->valid_tx_ant;
	if (data->valid_rx_ant)
		rx_chains &= data->valid_rx_ant;

	sku = iwl_get_sku(cfg, nvm_sw);
	data->sku_cap_band_24GHz_enable = sku & NVM_SKU_CAP_BAND_24GHZ;
	data->sku_cap_band_52GHz_enable = sku & NVM_SKU_CAP_BAND_52GHZ;
	data->sku_cap_11n_enable = sku & NVM_SKU_CAP_11N_ENABLE;
	data->sku_cap_11ac_enable = sku & NVM_SKU_CAP_11AC_ENABLE;
	if (iwlwifi_mod_params.disable_11n & IWL_DISABLE_HT_ALL)
		data->sku_cap_11n_enable = false;

	data->n_hw_addrs = iwl_get_n_hw_addrs(cfg, nvm_sw);

	if (cfg->device_family != IWL_DEVICE_FAMILY_8000) {
		/* Checking for required sections */
		if (!nvm_calib) {
			IWL_ERR_DEV(dev,
				    "Can't parse empty Calib NVM sections\n");
			kfree(data);
			return NULL;
		}
		/* in family 8000 Xtal calibration values moved to OTP */
		data->xtal_calib[0] = *(nvm_calib + XTAL_CALIB);
		data->xtal_calib[1] = *(nvm_calib + XTAL_CALIB + 1);
	}

	if (cfg->device_family != IWL_DEVICE_FAMILY_8000) {
		iwl_set_hw_address(cfg, data, nvm_hw);

		iwl_init_sbands(dev, cfg, data, nvm_sw,
				sku & NVM_SKU_CAP_11AC_ENABLE, tx_chains,
				rx_chains, lar_supported);
	} else {
		/* MAC address in family 8000 */
		iwl_set_hw_address_family_8000(dev, cfg, data, mac_override,
					       nvm_hw);

		iwl_init_sbands(dev, cfg, data, regulatory,
				sku & NVM_SKU_CAP_11AC_ENABLE, tx_chains,
				rx_chains, lar_supported);
	}

	data->calib_version = 255;

	return data;
}
