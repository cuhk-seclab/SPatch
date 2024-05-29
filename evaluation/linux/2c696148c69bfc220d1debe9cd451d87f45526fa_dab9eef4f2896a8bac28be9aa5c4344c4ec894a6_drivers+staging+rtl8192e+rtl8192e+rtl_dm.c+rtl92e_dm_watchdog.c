void rtl92e_dm_watchdog(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if (priv->being_init_adapter)
		return;

	_rtl92e_dm_check_ac_dc_power(dev);

	_rtl92e_dm_check_pbc_gpio(dev);
	_rtl92e_dm_check_txrateandretrycount(dev);
	_rtl92e_dm_check_edca_turbo(dev);

	_rtl92e_dm_check_rate_adaptive(dev);
	dm_dynamic_txpower(dev);
	_rtl92e_dm_check_tx_power_tracking(dev);

	dm_ctrl_initgain_byrssi(dev);
	_rtl92e_dm_bandwidth_autoswitch(dev);

	_rtl92e_dm_check_rx_path_selection(dev);
	_rtl92e_dm_check_fsync(dev);

	dm_send_rssi_tofw(dev);
	_rtl92e_dm_cts_to_self(dev);
}
