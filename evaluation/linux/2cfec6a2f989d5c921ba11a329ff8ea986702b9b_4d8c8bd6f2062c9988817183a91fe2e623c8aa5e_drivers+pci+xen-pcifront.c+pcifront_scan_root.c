static int pcifront_scan_root(struct pcifront_device *pdev,
				 unsigned int domain, unsigned int bus)
{
	struct pci_bus *b;
	LIST_HEAD(resources);
	struct pcifront_sd *sd = NULL;
	struct pci_bus_entry *bus_entry = NULL;
	int err = 0;
	static struct resource busn_res = {
		.start = 0,
		.end = 255,
		.flags = IORESOURCE_BUS,
	};

#ifndef CONFIG_PCI_DOMAINS
	if (domain != 0) {
		dev_err(&pdev->xdev->dev,
			"PCI Root in non-zero PCI Domain! domain=%d\n", domain);
		dev_err(&pdev->xdev->dev,
			"Please compile with CONFIG_PCI_DOMAINS\n");
		err = -EINVAL;
		goto err_out;
	}
#endif

	dev_info(&pdev->xdev->dev, "Creating PCI Frontend Bus %04x:%02x\n",
		 domain, bus);

	bus_entry = kzalloc(sizeof(*bus_entry), GFP_KERNEL);
	sd = kzalloc(sizeof(*sd), GFP_KERNEL);
	if (!bus_entry || !sd) {
		err = -ENOMEM;
		goto err_out;
	}
	pci_add_resource(&resources, &ioport_resource);
	pci_add_resource(&resources, &iomem_resource);
	pci_add_resource(&resources, &busn_res);
	pcifront_init_sd(sd, domain, bus, pdev);

	pci_lock_rescan_remove();

	b = pci_scan_root_bus(&pdev->xdev->dev, bus,
				  &pcifront_bus_ops, sd, &resources);
	if (!b) {
		dev_err(&pdev->xdev->dev,
			"Error creating PCI Frontend Bus!\n");
		err = -ENOMEM;
		pci_unlock_rescan_remove();
		pci_free_resource_list(&resources);
		goto err_out;
	}

	bus_entry->bus = b;

	list_add(&bus_entry->list, &pdev->root_buses);

	/* pci_scan_root_bus skips devices which do not have a
	* devfn==0. The pcifront_scan_bus enumerates all devfn. */
	err = pcifront_scan_bus(pdev, domain, bus, b);

	/* Claim resources before going "live" with our devices */
	pci_walk_bus(b, pcifront_claim_resource, pdev);

	/* Create SysFS and notify udev of the devices. Aka: "going live" */
	pci_bus_add_devices(b);

	pci_unlock_rescan_remove();
	return err;

err_out:
	kfree(bus_entry);
	kfree(sd);

	return err;
}
