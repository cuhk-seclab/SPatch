static int tpm_tis_init(struct device *dev, struct tpm_info *tpm_info,
			acpi_handle acpi_dev_handle)
{
	u32 vendor, intfcaps, intmask;
	int rc, i, irq_s, irq_e, probe;
	int irq_r = -1;
	struct tpm_chip *chip;
	struct priv_data *priv;

	priv = devm_kzalloc(dev, sizeof(struct priv_data), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	chip = tpmm_chip_alloc(dev, &tpm_tis);
	if (IS_ERR(chip))
		return PTR_ERR(chip);

	chip->vendor.priv = priv;
#ifdef CONFIG_ACPI
	chip->acpi_dev_handle = acpi_dev_handle;
#endif

	chip->vendor.iobase = devm_ioremap(dev, tpm_info->start, tpm_info->len);
	if (!chip->vendor.iobase)
		return -EIO;

	/* Maximum timeouts */
	chip->vendor.timeout_a = TIS_TIMEOUT_A_MAX;
	chip->vendor.timeout_b = TIS_TIMEOUT_B_MAX;
	chip->vendor.timeout_c = TIS_TIMEOUT_C_MAX;
	chip->vendor.timeout_d = TIS_TIMEOUT_D_MAX;

	if (wait_startup(chip, 0) != 0) {
		rc = -ENODEV;
		goto out_err;
	}

	if (request_locality(chip, 0) != 0) {
		rc = -ENODEV;
		goto out_err;
	}

	rc = tpm2_probe(chip);
	if (rc)
		goto out_err;

	vendor = ioread32(chip->vendor.iobase + TPM_DID_VID(0));
	chip->vendor.manufacturer_id = vendor;

	dev_info(dev, "%s TPM (device-id 0x%X, rev-id %d)\n",
		 (chip->flags & TPM_CHIP_FLAG_TPM2) ? "2.0" : "1.2",
		 vendor >> 16, ioread8(chip->vendor.iobase + TPM_RID(0)));

	if (!itpm) {
		probe = probe_itpm(chip);
		if (probe < 0) {
			rc = -ENODEV;
			goto out_err;
		}
		itpm = !!probe;
	}

	if (itpm)
		dev_info(dev, "Intel iTPM workaround enabled\n");


	/* Figure out the capabilities */
	intfcaps =
	    ioread32(chip->vendor.iobase +
		     TPM_INTF_CAPS(chip->vendor.locality));
	dev_dbg(dev, "TPM interface capabilities (0x%x):\n",
		intfcaps);
	if (intfcaps & TPM_INTF_BURST_COUNT_STATIC)
		dev_dbg(dev, "\tBurst Count Static\n");
	if (intfcaps & TPM_INTF_CMD_READY_INT)
		dev_dbg(dev, "\tCommand Ready Int Support\n");
	if (intfcaps & TPM_INTF_INT_EDGE_FALLING)
		dev_dbg(dev, "\tInterrupt Edge Falling\n");
	if (intfcaps & TPM_INTF_INT_EDGE_RISING)
		dev_dbg(dev, "\tInterrupt Edge Rising\n");
	if (intfcaps & TPM_INTF_INT_LEVEL_LOW)
		dev_dbg(dev, "\tInterrupt Level Low\n");
	if (intfcaps & TPM_INTF_INT_LEVEL_HIGH)
		dev_dbg(dev, "\tInterrupt Level High\n");
	if (intfcaps & TPM_INTF_LOCALITY_CHANGE_INT)
		dev_dbg(dev, "\tLocality Change Int Support\n");
	if (intfcaps & TPM_INTF_STS_VALID_INT)
		dev_dbg(dev, "\tSts Valid Int Support\n");
	if (intfcaps & TPM_INTF_DATA_AVAIL_INT)
		dev_dbg(dev, "\tData Avail Int Support\n");

	/* INTERRUPT Setup */
	init_waitqueue_head(&chip->vendor.read_queue);
	init_waitqueue_head(&chip->vendor.int_queue);

	intmask =
	    ioread32(chip->vendor.iobase +
		     TPM_INT_ENABLE(chip->vendor.locality));

	intmask |= TPM_INTF_CMD_READY_INT
	    | TPM_INTF_LOCALITY_CHANGE_INT | TPM_INTF_DATA_AVAIL_INT
	    | TPM_INTF_STS_VALID_INT;

	iowrite32(intmask,
		  chip->vendor.iobase +
		  TPM_INT_ENABLE(chip->vendor.locality));
	if (interrupts)
		chip->vendor.irq = tpm_info->irq;
	if (interrupts && !chip->vendor.irq) {
		irq_s =
		    ioread8(chip->vendor.iobase +
			    TPM_INT_VECTOR(chip->vendor.locality));
		irq_r = irq_s;
		if (irq_s) {
			irq_e = irq_s;
		} else {
			irq_s = 3;
			irq_e = 15;
		}

		for (i = irq_s; i <= irq_e && chip->vendor.irq == 0; i++) {
			iowrite8(i, chip->vendor.iobase +
				 TPM_INT_VECTOR(chip->vendor.locality));
			if (devm_request_irq
			    (dev, i, tis_int_probe, IRQF_SHARED,
			     chip->devname, chip) != 0) {
				dev_info(chip->pdev,
					 "Unable to request irq: %d for probe\n",
					 i);
				continue;
			}

			/* Clear all existing */
			iowrite32(ioread32
				  (chip->vendor.iobase +
				   TPM_INT_STATUS(chip->vendor.locality)),
				  chip->vendor.iobase +
				  TPM_INT_STATUS(chip->vendor.locality));

			/* Turn on */
			iowrite32(intmask | TPM_GLOBAL_INT_ENABLE,
				  chip->vendor.iobase +
				  TPM_INT_ENABLE(chip->vendor.locality));

			chip->vendor.probed_irq = 0;

			/* Generate Interrupts */
			if (chip->flags & TPM_CHIP_FLAG_TPM2)
				tpm2_gen_interrupt(chip);
			else
				tpm_gen_interrupt(chip);

			chip->vendor.irq = chip->vendor.probed_irq;

			/* free_irq will call into tis_int_probe;
			   clear all irqs we haven't seen while doing
			   tpm_gen_interrupt */
			iowrite32(ioread32
				  (chip->vendor.iobase +
				   TPM_INT_STATUS(chip->vendor.locality)),
				  chip->vendor.iobase +
				  TPM_INT_STATUS(chip->vendor.locality));

			/* Turn off */
			iowrite32(intmask,
				  chip->vendor.iobase +
				  TPM_INT_ENABLE(chip->vendor.locality));

			devm_free_irq(dev, i, chip);
		}
	}
	if (chip->vendor.irq) {
		iowrite8(chip->vendor.irq,
			 chip->vendor.iobase +
			 TPM_INT_VECTOR(chip->vendor.locality));
		if (devm_request_irq
		    (dev, chip->vendor.irq, tis_int_handler, IRQF_SHARED,
		     chip->devname, chip) != 0) {
			dev_info(chip->pdev,
				 "Unable to request irq: %d for use\n",
				 chip->vendor.irq);
			chip->vendor.irq = 0;
		} else {
			/* Clear all existing */
			iowrite32(ioread32
				  (chip->vendor.iobase +
				   TPM_INT_STATUS(chip->vendor.locality)),
				  chip->vendor.iobase +
				  TPM_INT_STATUS(chip->vendor.locality));

			/* Turn on */
			iowrite32(intmask | TPM_GLOBAL_INT_ENABLE,
				  chip->vendor.iobase +
				  TPM_INT_ENABLE(chip->vendor.locality));
		}
	} else if (irq_r != -1)
		iowrite8(irq_r, chip->vendor.iobase +
			 TPM_INT_VECTOR(chip->vendor.locality));

	if (chip->flags & TPM_CHIP_FLAG_TPM2) {
		chip->vendor.timeout_a = msecs_to_jiffies(TPM2_TIMEOUT_A);
		chip->vendor.timeout_b = msecs_to_jiffies(TPM2_TIMEOUT_B);
		chip->vendor.timeout_c = msecs_to_jiffies(TPM2_TIMEOUT_C);
		chip->vendor.timeout_d = msecs_to_jiffies(TPM2_TIMEOUT_D);
		chip->vendor.duration[TPM_SHORT] =
			msecs_to_jiffies(TPM2_DURATION_SHORT);
		chip->vendor.duration[TPM_MEDIUM] =
			msecs_to_jiffies(TPM2_DURATION_MEDIUM);
		chip->vendor.duration[TPM_LONG] =
			msecs_to_jiffies(TPM2_DURATION_LONG);

		rc = tpm2_do_selftest(chip);
		if (rc == TPM2_RC_INITIALIZE) {
			dev_warn(dev, "Firmware has not started TPM\n");
			rc  = tpm2_startup(chip, TPM2_SU_CLEAR);
			if (!rc)
				rc = tpm2_do_selftest(chip);
		}

		if (rc) {
			dev_err(dev, "TPM self test failed\n");
			if (rc > 0)
				rc = -ENODEV;
			goto out_err;
		}
	} else {
		if (tpm_get_timeouts(chip)) {
			dev_err(dev, "Could not get TPM timeouts and durations\n");
			rc = -ENODEV;
			goto out_err;
		}

		if (tpm_do_selftest(chip)) {
			dev_err(dev, "TPM self test failed\n");
			rc = -ENODEV;
			goto out_err;
		}
	}

	return tpm_chip_register(chip);
out_err:
	tpm_tis_remove(chip);
	return rc;
}
