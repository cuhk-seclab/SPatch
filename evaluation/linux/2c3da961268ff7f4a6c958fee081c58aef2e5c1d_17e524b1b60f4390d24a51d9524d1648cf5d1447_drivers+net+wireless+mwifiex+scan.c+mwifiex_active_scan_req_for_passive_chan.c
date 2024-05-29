static int
mwifiex_active_scan_req_for_passive_chan(struct mwifiex_private *priv)
{
	int ret;
	struct mwifiex_adapter *adapter = priv->adapter;
	u8 id = 0;
	struct mwifiex_user_scan_cfg  *user_scan_cfg;

	if (adapter->active_scan_triggered || !priv->scan_request) {
		adapter->active_scan_triggered = false;
		return 0;
	}

	if (!priv->hidden_chan[0].chan_number) {
		mwifiex_dbg(adapter, INFO, "No BSS with hidden SSID found on DFS channels\n");
		return 0;
	}
	user_scan_cfg = kzalloc(sizeof(*user_scan_cfg), GFP_KERNEL);

	if (!user_scan_cfg)
		return -ENOMEM;

	memset(user_scan_cfg, 0, sizeof(*user_scan_cfg));

	for (id = 0; id < MWIFIEX_USER_SCAN_CHAN_MAX; id++) {
		if (!priv->hidden_chan[id].chan_number)
			break;
		memcpy(&user_scan_cfg->chan_list[id],
		       &priv->hidden_chan[id],
		       sizeof(struct mwifiex_user_scan_chan));
	}

	adapter->active_scan_triggered = true;
	user_scan_cfg->num_ssids = priv->scan_request->n_ssids;
	user_scan_cfg->ssid_list = priv->scan_request->ssids;

	ret = mwifiex_scan_networks(priv, user_scan_cfg);
	kfree(user_scan_cfg);

	memset(&priv->hidden_chan, 0, sizeof(priv->hidden_chan));

	if (ret) {
		dev_err(priv->adapter->dev, "scan failed: %d\n", ret);
		return ret;
	}

	return 0;
}
