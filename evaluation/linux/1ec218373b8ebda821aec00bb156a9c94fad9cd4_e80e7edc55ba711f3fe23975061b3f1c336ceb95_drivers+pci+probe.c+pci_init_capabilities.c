static void pci_init_capabilities(struct pci_dev *dev)
{
	/* Enhanced Allocation */
	pci_ea_init(dev);

	/* MSI/MSI-X list */
	pci_msi_init_pci_dev(dev);

	/* Setup MSI caps & disable MSI/MSI-X interrupts */
	pci_msi_setup_pci_dev(dev);

	/* Buffers for saving PCIe and PCI-X capabilities */
	pci_allocate_cap_save_buffers(dev);

	/* Power Management */
	pci_pm_init(dev);

	/* Vital Product Data */
	pci_vpd_pci22_init(dev);

	/* Alternative Routing-ID Forwarding */
	pci_configure_ari(dev);

	/* Single Root I/O Virtualization */
	pci_iov_init(dev);

	/* Address Translation Services */
	pci_ats_init(dev);

	/* Enable ACS P2P upstream forwarding */
	pci_enable_acs(dev);

	pci_cleanup_aer_error_status_regs(dev);
}
