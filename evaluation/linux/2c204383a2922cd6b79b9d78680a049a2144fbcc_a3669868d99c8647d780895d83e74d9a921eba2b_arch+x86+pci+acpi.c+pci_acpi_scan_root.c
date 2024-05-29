struct pci_bus *pci_acpi_scan_root(struct acpi_pci_root *root)
{
	struct acpi_device *device = root->device;
	struct pci_root_info *info;
	int domain = root->segment;
	int busnum = root->secondary.start;
	struct resource_entry *res_entry;
	LIST_HEAD(crs_res);
	LIST_HEAD(resources);
	struct pci_bus *bus;
	struct pci_sysdata *sd;
	int node;

	if (pci_ignore_seg)
		root->segment = domain = 0;

	if (domain && !pci_domains_supported) {
		printk(KERN_WARNING "pci_bus %04x:%02x: "
		       "ignored (multiple domains not supported)\n",
		       domain, busnum);
		return NULL;
	}

	node = acpi_get_node(device->handle);
	if (node == NUMA_NO_NODE) {
		node = x86_pci_root_bus_node(busnum);
		if (node != 0 && node != NUMA_NO_NODE)
			dev_info(&device->dev, FW_BUG "no _PXM; falling back to node %d from hardware (may be inconsistent with ACPI node numbers)\n",
				node);
	}

	if (node != NUMA_NO_NODE && !node_online(node))
		node = NUMA_NO_NODE;

	info = kzalloc_node(sizeof(*info), GFP_KERNEL, node);
	if (!info) {
		printk(KERN_WARNING "pci_bus %04x:%02x: "
		       "ignored (out of memory)\n", domain, busnum);
		return NULL;
	}

	sd = &info->sd;
	sd->domain = domain;
	sd->node = node;
	sd->companion = device;

	bus = pci_find_bus(domain, busnum);
	if (bus) {
		/*
		 * If the desired bus has been scanned already, replace
		 * its bus->sysdata.
		 */
		memcpy(bus->sysdata, sd, sizeof(*sd));
		kfree(info);
	} else {
		/* insert busn res at first */
		pci_add_resource(&resources,  &root->secondary);

		/*
		 * _CRS with no apertures is normal, so only fall back to
		 * defaults or native bridge info if we're ignoring _CRS.
		 */
		probe_pci_root_info(info, device, busnum, domain, &crs_res);
		if (pci_use_crs) {
			add_resources(info, &resources, &crs_res);
		} else {
			resource_list_for_each_entry(res_entry, &crs_res)
				dev_printk(KERN_DEBUG, &device->dev,
					   "host bridge window %pR (ignored)\n",
					   res_entry->res);
			resource_list_free(&crs_res);
			x86_pci_root_bus_resources(busnum, &resources);
		}

		if (!setup_mcfg_map(info, domain, (u8)root->secondary.start,
				    (u8)root->secondary.end, root->mcfg_addr))
			bus = pci_create_root_bus(NULL, busnum, &pci_root_ops,
						  sd, &resources);

		if (bus) {
			pci_scan_child_bus(bus);
			pci_set_host_bridge_release(
				to_pci_host_bridge(bus->bridge),
				release_pci_root_info, info);
		} else {
			resource_list_free(&resources);
			teardown_mcfg_map(info);
			kfree(info);
		}
	}

	/* After the PCI-E bus has been walked and all devices discovered,
	 * configure any settings of the fabric that might be necessary.
	 */
	if (bus) {
		struct pci_bus *child;
		list_for_each_entry(child, &bus->children, node)
			pcie_bus_configure_settings(child);
	}

	if (bus && node != NUMA_NO_NODE)
		dev_printk(KERN_DEBUG, &bus->dev, "on NUMA node %d\n", node);

	return bus;
}
