static void rtl8192_init_priv_task(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	priv->priv_wq = create_workqueue(DRV_NAME);
	INIT_WORK_RSL(&priv->reset_wq, (void *)rtl8192_restart, dev);
	INIT_WORK_RSL(&priv->rtllib->ips_leave_wq, (void *)rtl92e_ips_leave_wq,
		      dev);
	INIT_DELAYED_WORK_RSL(&priv->watch_dog_wq,
			      (void *)rtl819x_watchdog_wqcallback, dev);
	INIT_DELAYED_WORK_RSL(&priv->txpower_tracking_wq,
			      (void *)rtl92e_dm_txpower_tracking_wq, dev);
	INIT_DELAYED_WORK_RSL(&priv->rfpath_check_wq,
			      (void *)rtl92e_dm_rf_pathcheck_wq, dev);
	INIT_DELAYED_WORK_RSL(&priv->update_beacon_wq,
			      (void *)rtl8192_update_beacon, dev);
	INIT_WORK_RSL(&priv->qos_activate, (void *)rtl8192_qos_activate, dev);
	INIT_DELAYED_WORK_RSL(&priv->rtllib->hw_wakeup_wq,
			      (void *) rtl8192_hw_wakeup_wq, dev);
	INIT_DELAYED_WORK_RSL(&priv->rtllib->hw_sleep_wq,
			      (void *) rtl8192_hw_sleep_wq, dev);
	tasklet_init(&priv->irq_rx_tasklet,
		     (void(*)(unsigned long))rtl8192_irq_rx_tasklet,
		     (unsigned long)priv);
	tasklet_init(&priv->irq_tx_tasklet,
		     (void(*)(unsigned long))rtl8192_irq_tx_tasklet,
		     (unsigned long)priv);
	tasklet_init(&priv->irq_prepare_beacon_tasklet,
		     (void(*)(unsigned long))rtl8192_prepare_beacon,
		     (unsigned long)priv);
}
