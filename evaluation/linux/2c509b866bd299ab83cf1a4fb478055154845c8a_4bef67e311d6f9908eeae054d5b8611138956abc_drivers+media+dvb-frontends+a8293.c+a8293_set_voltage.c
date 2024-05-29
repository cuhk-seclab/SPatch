static int a8293_set_voltage(struct dvb_frontend *fe,
	enum fe_sec_voltage fe_sec_voltage)
{
	struct a8293_dev *dev = fe->sec_priv;
	struct i2c_client *client = dev->client;
	int ret;
	u8 reg0, reg1;

	dev_dbg(&client->dev, "fe_sec_voltage=%d\n", fe_sec_voltage);

	switch (fe_sec_voltage) {
	case SEC_VOLTAGE_OFF:
		/* ENB=0 */
		reg0 = 0x10;
		break;
	case SEC_VOLTAGE_13:
		/* VSEL0=1, VSEL1=0, VSEL2=0, VSEL3=0, ENB=1*/
		reg0 = 0x31;
		break;
	case SEC_VOLTAGE_18:
		/* VSEL0=0, VSEL1=0, VSEL2=0, VSEL3=1, ENB=1*/
		reg0 = 0x38;
		break;
	default:
		ret = -EINVAL;
		goto err;
	}
	if (reg0 != dev->reg[0]) {
		ret = i2c_master_send(client, &reg0, 1);
		if (ret < 0)
			goto err;
		dev->reg[0] = reg0;
	}

	/* TMODE=0, TGATE=1 */
	reg1 = 0x82;
	if (reg1 != dev->reg[1]) {
		ret = i2c_master_send(client, &reg1, 1);
		if (ret < 0)
			goto err;
		dev->reg[1] = reg1;
	}

	usleep_range(1500, 50000);
	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}
