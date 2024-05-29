static int dvbsky_s960c_attach(struct dvb_usb_adapter *adap)
{
	struct dvbsky_state *state = adap_to_priv(adap);
	struct dvb_usb_device *d = adap_to_d(adap);
	int ret = 0;
	/* demod I2C adapter */
	struct i2c_adapter *i2c_adapter;
	struct i2c_client *client_tuner, *client_ci;
	struct i2c_board_info info;
	struct sp2_config sp2_config;
	struct m88ts2022_config m88ts2022_config = {
			.clock = 27000000,
		};
	memset(&info, 0, sizeof(struct i2c_board_info));

	/* attach demod */
	adap->fe[0] = dvb_attach(m88ds3103_attach,
			&dvbsky_s960c_m88ds3103_config,
			&d->i2c_adap,
			&i2c_adapter);
	if (!adap->fe[0]) {
		dev_err(&d->udev->dev, "dvbsky_s960ci_attach fail.\n");
		ret = -ENODEV;
		goto fail_attach;
	}

	/* attach tuner */
	m88ts2022_config.fe = adap->fe[0];
	strlcpy(info.type, "m88ts2022", I2C_NAME_SIZE);
	info.addr = 0x60;
	info.platform_data = &m88ts2022_config;
	request_module("m88ts2022");
	client_tuner = i2c_new_device(i2c_adapter, &info);
	if (client_tuner == NULL || client_tuner->dev.driver == NULL) {
		ret = -ENODEV;
		goto fail_tuner_device;
	}

	if (!try_module_get(client_tuner->dev.driver->owner)) {
		ret = -ENODEV;
		goto fail_tuner_module;
	}

	/* attach ci controller */
	memset(&sp2_config, 0, sizeof(sp2_config));
	sp2_config.dvb_adap = &adap->dvb_adap;
	sp2_config.priv = d;
	sp2_config.ci_control = dvbsky_ci_ctrl;
	memset(&info, 0, sizeof(struct i2c_board_info));
	strlcpy(info.type, "sp2", I2C_NAME_SIZE);
	info.addr = 0x40;
	info.platform_data = &sp2_config;
	request_module("sp2");
	client_ci = i2c_new_device(&d->i2c_adap, &info);
	if (client_ci == NULL || client_ci->dev.driver == NULL) {
		ret = -ENODEV;
		goto fail_ci_device;
	}

	if (!try_module_get(client_ci->dev.driver->owner)) {
		ret = -ENODEV;
		goto fail_ci_module;
	}

	/* delegate signal strength measurement to tuner */
	adap->fe[0]->ops.read_signal_strength =
			adap->fe[0]->ops.tuner_ops.get_rf_strength;

	/* hook fe: need to resync the slave fifo when signal locks. */
	state->fe_read_status = adap->fe[0]->ops.read_status;
	adap->fe[0]->ops.read_status = dvbsky_usb_read_status;

	/* hook fe: LNB off/on is control by Cypress usb chip. */
	state->fe_set_voltage = adap->fe[0]->ops.set_voltage;
	adap->fe[0]->ops.set_voltage = dvbsky_usb_ci_set_voltage;

	state->i2c_client_tuner = client_tuner;
	state->i2c_client_ci = client_ci;
	return ret;
fail_ci_module:
	i2c_unregister_device(client_ci);
fail_ci_device:
	module_put(client_tuner->dev.driver->owner);
fail_tuner_module:
	i2c_unregister_device(client_tuner);
fail_tuner_device:
	dvb_frontend_detach(adap->fe[0]);
fail_attach:
	return ret;
}
