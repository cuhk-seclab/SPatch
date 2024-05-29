static pci_ers_result_t tg3_io_error_detected(struct pci_dev *pdev,
					      pci_channel_state_t state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct tg3 *tp = netdev_priv(netdev);
	pci_ers_result_t err = PCI_ERS_RESULT_NEED_RESET;

	netdev_info(netdev, "PCI I/O error detected\n");

	rtnl_lock();

	/* We needn't recover from permanent error */
	if (state == pci_channel_io_frozen)
		tp->pcierr_recovery = true;

	/* We probably don't have netdev yet */
	if (!netdev || !netif_running(netdev))
		goto done;

	tg3_phy_stop(tp);

	tg3_netif_stop(tp);

	tg3_timer_stop(tp);

	/* Want to make sure that the reset task doesn't run */
	tg3_reset_task_cancel(tp);

	netif_device_detach(netdev);

	/* Clean up software state, even if MMIO is blocked */
	tg3_full_lock(tp, 0);
	tg3_halt(tp, RESET_KIND_SHUTDOWN, 0);
	tg3_full_unlock(tp);

done:
	if (state == pci_channel_io_perm_failure) {
		if (netdev) {
			tg3_napi_enable(tp);
			dev_close(netdev);
		}
		err = PCI_ERS_RESULT_DISCONNECT;
	} else {
		pci_disable_device(pdev);
	}

	rtnl_unlock();

	return err;
}
