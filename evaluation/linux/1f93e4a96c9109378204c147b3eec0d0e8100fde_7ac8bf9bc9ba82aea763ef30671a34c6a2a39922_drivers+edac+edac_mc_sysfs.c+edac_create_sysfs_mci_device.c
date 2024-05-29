int edac_create_sysfs_mci_device(struct mem_ctl_info *mci,
				 const struct attribute_group **groups)
{
	int i, err;

	/*
	 * The memory controller needs its own bus, in order to avoid
	 * namespace conflicts at /sys/bus/edac.
	 */
	mci->bus->name = kasprintf(GFP_KERNEL, "mc%d", mci->mc_idx);
	if (!mci->bus->name)
		return -ENOMEM;

	edac_dbg(0, "creating bus %s\n", mci->bus->name);

	err = bus_register(mci->bus);
	if (err < 0)
		goto fail_free_name;

	/* get the /sys/devices/system/edac subsys reference */
	mci->dev.type = &mci_attr_type;
	device_initialize(&mci->dev);

	mci->dev.parent = mci_pdev;
	mci->dev.bus = mci->bus;
	mci->dev.groups = groups;
	dev_set_name(&mci->dev, "mc%d", mci->mc_idx);
	dev_set_drvdata(&mci->dev, mci);
	pm_runtime_forbid(&mci->dev);

	edac_dbg(0, "creating device %s\n", dev_name(&mci->dev));
	err = device_add(&mci->dev);
	if (err < 0) {
		edac_dbg(1, "failure: create device %s\n", dev_name(&mci->dev));
		goto fail_unregister_bus;
	}

	/*
	 * Create the dimm/rank devices
	 */
	for (i = 0; i < mci->tot_dimms; i++) {
		struct dimm_info *dimm = mci->dimms[i];
		/* Only expose populated DIMMs */
		if (!dimm->nr_pages)
			continue;

#ifdef CONFIG_EDAC_DEBUG
		edac_dbg(1, "creating dimm%d, located at ", i);
		if (edac_debug_level >= 1) {
			int lay;
			for (lay = 0; lay < mci->n_layers; lay++)
				printk(KERN_CONT "%s %d ",
					edac_layer_name[mci->layers[lay].type],
					dimm->location[lay]);
			printk(KERN_CONT "\n");
		}
#endif
		err = edac_create_dimm_object(mci, dimm, i);
		if (err) {
			edac_dbg(1, "failure: create dimm %d obj\n", i);
			goto fail_unregister_dimm;
		}
	}

#ifdef CONFIG_EDAC_LEGACY_SYSFS
	err = edac_create_csrow_objects(mci);
	if (err < 0)
		goto fail_unregister_dimm;
#endif

	edac_create_debugfs_nodes(mci);
	return 0;

fail_unregister_dimm:
	for (i--; i >= 0; i--) {
		struct dimm_info *dimm = mci->dimms[i];
		if (!dimm->nr_pages)
			continue;

		device_unregister(&dimm->dev);
	}
	device_unregister(&mci->dev);
fail_unregister_bus:
	bus_unregister(mci->bus);
fail_free_name:
	kfree(mci->bus->name);
	return err;
}
