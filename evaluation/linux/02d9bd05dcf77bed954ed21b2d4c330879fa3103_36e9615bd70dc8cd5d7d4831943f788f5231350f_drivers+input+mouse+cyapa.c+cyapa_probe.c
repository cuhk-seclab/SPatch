static int cyapa_probe(struct i2c_client *client,
		       const struct i2c_device_id *dev_id)
{
	struct device *dev = &client->dev;
	struct cyapa *cyapa;
	u8 adapter_func;
	union i2c_smbus_data dummy;
	int error;

	adapter_func = cyapa_check_adapter_functionality(client);
	if (adapter_func == CYAPA_ADAPTER_FUNC_NONE) {
		dev_err(dev, "not a supported I2C/SMBus adapter\n");
		return -EIO;
	}

	/* Make sure there is something at this address */
	if (i2c_smbus_xfer(client->adapter, client->addr, 0,
			I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &dummy) < 0)
		return -ENODEV;

	cyapa = devm_kzalloc(dev, sizeof(struct cyapa), GFP_KERNEL);
	if (!cyapa)
		return -ENOMEM;

	/* i2c isn't supported, use smbus */
	if (adapter_func == CYAPA_ADAPTER_FUNC_SMBUS)
		cyapa->smbus = true;

	cyapa->client = client;
	i2c_set_clientdata(client, cyapa);
	sprintf(cyapa->phys, "i2c-%d-%04x/input0", client->adapter->nr,
		client->addr);

	cyapa->vcc = devm_regulator_get(dev, "vcc");
	if (IS_ERR(cyapa->vcc)) {
		error = PTR_ERR(cyapa->vcc);
		dev_err(dev, "failed to get vcc regulator: %d\n", error);
		return error;
	}

	error = regulator_enable(cyapa->vcc);
	if (error) {
		dev_err(dev, "failed to enable regulator: %d\n", error);
		return error;
	}

	error = devm_add_action(dev, cyapa_disable_regulator, cyapa);
	if (error) {
		cyapa_disable_regulator(cyapa);
		dev_err(dev, "failed to add disable regulator action: %d\n",
			error);
		return error;
	}

	error = cyapa_initialize(cyapa);
	if (error) {
		dev_err(dev, "failed to detect and initialize tp device.\n");
		return error;
	}

	error = sysfs_create_group(&client->dev.kobj, &cyapa_sysfs_group);
	if (error) {
		dev_err(dev, "failed to create sysfs entries: %d\n", error);
		return error;
	}

	error = devm_add_action(dev, cyapa_remove_sysfs_group, cyapa);
	if (error) {
		cyapa_remove_sysfs_group(cyapa);
		dev_err(dev, "failed to add sysfs cleanup action: %d\n", error);
		return error;
	}

	error = cyapa_prepare_wakeup_controls(cyapa);
	if (error) {
		dev_err(dev, "failed to prepare wakeup controls: %d\n", error);
		return error;
	}

	error = cyapa_start_runtime(cyapa);
	if (error) {
		dev_err(dev, "failed to start pm_runtime: %d\n", error);
		return error;
	}

	error = devm_request_threaded_irq(dev, client->irq,
					  NULL, cyapa_irq,
					  IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					  "cyapa", cyapa);
	if (error) {
		dev_err(dev, "failed to request threaded irq: %d\n", error);
		return error;
	}

	/* Disable IRQ until the device is opened */
	disable_irq(client->irq);

	/*
	 * Register the device in the input subsystem when it's operational.
	 * Otherwise, keep in this driver, so it can be be recovered or updated
	 * through the sysfs mode and update_fw interfaces by user or apps.
	 */
	if (cyapa->operational) {
		error = cyapa_create_input_dev(cyapa);
		if (error) {
			dev_err(dev, "create input_dev instance failed: %d\n",
					error);
			return error;
		}
	}

	return 0;
}
