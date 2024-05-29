void mwifiex_drv_info_dump(struct mwifiex_adapter *adapter)
{
	void *p;
	char drv_version[64];
	struct usb_card_rec *cardp;
	struct sdio_mmc_card *sdio_card;
	struct mwifiex_private *priv;
	int i, idx;
	struct netdev_queue *txq;
	struct mwifiex_debug_info *debug_info;

	if (adapter->drv_info_dump) {
		vfree(adapter->drv_info_dump);
		adapter->drv_info_dump = NULL;
		adapter->drv_info_size = 0;
	}

	mwifiex_dbg(adapter, MSG, "===mwifiex driverinfo dump start===\n");

	adapter->drv_info_dump = vzalloc(MWIFIEX_DRV_INFO_SIZE_MAX);

	if (!adapter->drv_info_dump)
		return;

	p = (char *)(adapter->drv_info_dump);
	p += sprintf(p, "driver_name = " "\"mwifiex\"\n");

	mwifiex_drv_get_driver_version(adapter, drv_version,
				       sizeof(drv_version) - 1);
	p += sprintf(p, "driver_version = %s\n", drv_version);

	if (adapter->iface_type == MWIFIEX_USB) {
		cardp = (struct usb_card_rec *)adapter->card;
		p += sprintf(p, "tx_cmd_urb_pending = %d\n",
			     atomic_read(&cardp->tx_cmd_urb_pending));
		p += sprintf(p, "tx_data_urb_pending_port_0 = %d\n",
			     atomic_read(&cardp->port[0].tx_data_urb_pending));
		p += sprintf(p, "tx_data_urb_pending_port_1 = %d\n",
			     atomic_read(&cardp->port[1].tx_data_urb_pending));
		p += sprintf(p, "rx_cmd_urb_pending = %d\n",
			     atomic_read(&cardp->rx_cmd_urb_pending));
		p += sprintf(p, "rx_data_urb_pending = %d\n",
			     atomic_read(&cardp->rx_data_urb_pending));
	}

	p += sprintf(p, "tx_pending = %d\n",
		     atomic_read(&adapter->tx_pending));
	p += sprintf(p, "rx_pending = %d\n",
		     atomic_read(&adapter->rx_pending));

	if (adapter->iface_type == MWIFIEX_SDIO) {
		sdio_card = (struct sdio_mmc_card *)adapter->card;
		p += sprintf(p, "\nmp_rd_bitmap=0x%x curr_rd_port=0x%x\n",
			     sdio_card->mp_rd_bitmap, sdio_card->curr_rd_port);
		p += sprintf(p, "mp_wr_bitmap=0x%x curr_wr_port=0x%x\n",
			     sdio_card->mp_wr_bitmap, sdio_card->curr_wr_port);
	}

	for (i = 0; i < adapter->priv_num; i++) {
		if (!adapter->priv[i] || !adapter->priv[i]->netdev)
			continue;
		priv = adapter->priv[i];
		p += sprintf(p, "\n[interface  : \"%s\"]\n",
			     priv->netdev->name);
		p += sprintf(p, "wmm_tx_pending[0] = %d\n",
			     atomic_read(&priv->wmm_tx_pending[0]));
		p += sprintf(p, "wmm_tx_pending[1] = %d\n",
			     atomic_read(&priv->wmm_tx_pending[1]));
		p += sprintf(p, "wmm_tx_pending[2] = %d\n",
			     atomic_read(&priv->wmm_tx_pending[2]));
		p += sprintf(p, "wmm_tx_pending[3] = %d\n",
			     atomic_read(&priv->wmm_tx_pending[3]));
		p += sprintf(p, "media_state=\"%s\"\n", !priv->media_connected ?
			     "Disconnected" : "Connected");
		p += sprintf(p, "carrier %s\n", (netif_carrier_ok(priv->netdev)
			     ? "on" : "off"));
		for (idx = 0; idx < priv->netdev->num_tx_queues; idx++) {
			txq = netdev_get_tx_queue(priv->netdev, idx);
			p += sprintf(p, "tx queue %d:%s  ", idx,
				     netif_tx_queue_stopped(txq) ?
				     "stopped" : "started");
		}
		p += sprintf(p, "\n%s: num_tx_timeout = %d\n",
			     priv->netdev->name, priv->num_tx_timeout);
	}

	if (adapter->iface_type == MWIFIEX_SDIO) {
		p += sprintf(p, "\n=== SDIO register dump===\n");
		if (adapter->if_ops.reg_dump)
			p += adapter->if_ops.reg_dump(adapter, p);
	}

	p += sprintf(p, "\n=== more debug information\n");
	debug_info = kzalloc(sizeof(*debug_info), GFP_KERNEL);
	if (debug_info) {
		for (i = 0; i < adapter->priv_num; i++) {
			if (!adapter->priv[i] || !adapter->priv[i]->netdev)
				continue;
			priv = adapter->priv[i];
			mwifiex_get_debug_info(priv, debug_info);
			p += mwifiex_debug_info_to_buffer(priv, p, debug_info);
			break;
		}
		kfree(debug_info);
	}

	adapter->drv_info_size = p - adapter->drv_info_dump;
	mwifiex_dbg(adapter, MSG, "===mwifiex driverinfo dump end===\n");
}
