static int r852_register_nand_device(struct r852_device *dev)
{
	struct mtd_info *mtd = nand_to_mtd(dev->chip);

	WARN_ON(dev->card_registred);

	mtd->dev.parent = &dev->pci_dev->dev;

	if (dev->readonly)
		dev->chip->options |= NAND_ROM;

	r852_engine_enable(dev);

	if (sm_register_device(mtd, dev->sm))
		goto error1;

	if (device_create_file(&mtd->dev, &dev_attr_media_type)) {
		message("can't create media type sysfs attribute");
		goto error3;
	}

	dev->card_registred = 1;
	return 0;
error3:
	nand_release(mtd);
error1:
	/* Force card redetect */
	dev->card_detected = 0;
	return -1;
}
