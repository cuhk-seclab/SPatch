static void efx_fini_io(struct efx_nic *efx)
{
	int bar;

	netif_dbg(efx, drv, efx->net_dev, "shutting down I/O\n");

	if (efx->membase) {
		iounmap(efx->membase);
		efx->membase = NULL;
	}

	if (efx->membase_phys) {
		bar = efx->type->mem_bar;
		pci_release_region(efx->pci_dev, bar);
		efx->membase_phys = 0;
	}

	/* Don't disable bus-mastering if VFs are assigned */
	if (!pci_vfs_assigned(efx->pci_dev))
		pci_disable_device(efx->pci_dev);
}
