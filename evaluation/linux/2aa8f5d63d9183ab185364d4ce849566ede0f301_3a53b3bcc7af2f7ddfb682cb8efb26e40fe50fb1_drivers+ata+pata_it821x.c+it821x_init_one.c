static int it821x_init_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	u8 conf;

	static const struct ata_port_info info_smart = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = ATA_PIO4,
		.mwdma_mask = ATA_MWDMA2,
		.udma_mask = ATA_UDMA6,
		.port_ops = &it821x_smart_port_ops
	};
	static const struct ata_port_info info_passthru = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = ATA_PIO4,
		.mwdma_mask = ATA_MWDMA2,
		.udma_mask = ATA_UDMA6,
		.port_ops = &it821x_passthru_port_ops
	};
	static const struct ata_port_info info_rdc = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = ATA_PIO4,
		.mwdma_mask = ATA_MWDMA2,
		.udma_mask = ATA_UDMA6,
		.port_ops = &it821x_rdc_port_ops
	};
	static const struct ata_port_info info_rdc_11 = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = ATA_PIO4,
		.mwdma_mask = ATA_MWDMA2,
		/* No UDMA */
		.port_ops = &it821x_rdc_port_ops
	};

	const struct ata_port_info *ppi[] = { NULL, NULL };
	static const char *mode[2] = { "pass through", "smart" };
	int rc;

	rc = pcim_enable_device(pdev);
	if (rc)
		return rc;

	if (pdev->vendor == PCI_VENDOR_ID_RDC) {
		/* Deal with Vortex86SX */
		if (pdev->revision == 0x11)
			ppi[0] = &info_rdc_11;
		else
			ppi[0] = &info_rdc;
	} else {
		/* Force the card into bypass mode if so requested */
		if (it8212_noraid) {
			printk(KERN_INFO DRV_NAME ": forcing bypass mode.\n");
			it821x_disable_raid(pdev);
		}
		pci_read_config_byte(pdev, 0x50, &conf);
		conf &= 1;

		printk(KERN_INFO DRV_NAME": controller in %s mode.\n",
								mode[conf]);
		if (conf == 0)
			ppi[0] = &info_passthru;
		else
			ppi[0] = &info_smart;
	}
	return ata_pci_bmdma_init_one(pdev, ppi, &it821x_sht, NULL, 0);
}
