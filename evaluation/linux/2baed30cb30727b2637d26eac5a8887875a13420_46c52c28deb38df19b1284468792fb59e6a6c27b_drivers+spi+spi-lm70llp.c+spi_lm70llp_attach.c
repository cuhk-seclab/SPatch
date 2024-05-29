static void spi_lm70llp_attach(struct parport *p)
{
	struct pardevice	*pd;
	struct spi_lm70llp	*pp;
	struct spi_master	*master;
	int			status;
	struct pardev_cb	lm70llp_cb;

	if (lm70llp) {
		printk(KERN_WARNING
			"%s: spi_lm70llp instance already loaded. Aborting.\n",
			DRVNAME);
		return;
	}

	/* TODO:  this just _assumes_ a lm70 is there ... no probe;
	 * the lm70 driver could verify it, reading the manf ID.
	 */

	master = spi_alloc_master(p->physport->dev, sizeof *pp);
	if (!master) {
		status = -ENOMEM;
		goto out_fail;
	}
	pp = spi_master_get_devdata(master);

	/*
	 * SPI and bitbang hookup.
	 */
	pp->bitbang.master = master;
	pp->bitbang.chipselect = lm70_chipselect;
	pp->bitbang.txrx_word[SPI_MODE_0] = lm70_txrx;
	pp->bitbang.flags = SPI_3WIRE;

	/*
	 * Parport hookup
	 */
	pp->port = p;
	memset(&lm70llp_cb, 0, sizeof(lm70llp_cb));
	lm70llp_cb.private = pp;
	lm70llp_cb.flags = PARPORT_FLAG_EXCL;
	pd = parport_register_dev_model(p, DRVNAME, &lm70llp_cb, 0);

	if (!pd) {
		status = -ENOMEM;
		goto out_free_master;
	}
	pp->pd = pd;

	status = parport_claim(pd);
	if (status < 0)
		goto out_parport_unreg;

	/*
	 * Start SPI ...
	 */
	status = spi_bitbang_start(&pp->bitbang);
	if (status < 0) {
		dev_warn(&pd->dev, "spi_bitbang_start failed with status %d\n",
			 status);
		goto out_off_and_release;
	}

	/*
	 * The modalias name MUST match the device_driver name
	 * for the bus glue code to match and subsequently bind them.
	 * We are binding to the generic drivers/hwmon/lm70.c device
	 * driver.
	 */
	strcpy(pp->info.modalias, "lm70");
	pp->info.max_speed_hz = 6 * 1000 * 1000;
	pp->info.chip_select = 0;
	pp->info.mode = SPI_3WIRE | SPI_MODE_0;

	/* power up the chip, and let the LM70 control SI/SO */
	parport_write_data(pp->port, lm70_INIT);

	/* Enable access to our primary data structure via
	 * the board info's (void *)controller_data.
	 */
	pp->info.controller_data = pp;
	pp->spidev_lm70 = spi_new_device(pp->bitbang.master, &pp->info);
	if (pp->spidev_lm70)
		dev_dbg(&pp->spidev_lm70->dev, "spidev_lm70 at %s\n",
			dev_name(&pp->spidev_lm70->dev));
	else {
		dev_warn(&pd->dev, "spi_new_device failed\n");
		status = -ENODEV;
		goto out_bitbang_stop;
	}
	pp->spidev_lm70->bits_per_word = 8;

	lm70llp = pp;
	return;

out_bitbang_stop:
	spi_bitbang_stop(&pp->bitbang);
out_off_and_release:
	/* power down */
	parport_write_data(pp->port, 0);
	mdelay(10);
	parport_release(pp->pd);
out_parport_unreg:
	parport_unregister_device(pd);
out_free_master:
	spi_master_put(master);
out_fail:
	pr_info("%s: spi_lm70llp probe fail, status %d\n", DRVNAME, status);
}
