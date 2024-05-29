static int af9013_set_frontend(struct dvb_frontend *fe)
{
	struct af9013_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret, i, sampling_freq;
	bool auto_mode, spec_inv;
	u8 buf[6];
	u32 if_frequency, freq_cw;

	dev_dbg(&state->i2c->dev, "%s: frequency=%d bandwidth_hz=%d\n",
			__func__, c->frequency, c->bandwidth_hz);

	/* program tuner */
	if (fe->ops.tuner_ops.set_params)
		fe->ops.tuner_ops.set_params(fe);

	/* program CFOE coefficients */
	if (c->bandwidth_hz != state->bandwidth_hz) {
		for (i = 0; i < ARRAY_SIZE(coeff_lut); i++) {
			if (coeff_lut[i].clock == state->config.clock &&
				coeff_lut[i].bandwidth_hz == c->bandwidth_hz) {
				break;
			}
		}

		/* Return an error if can't find bandwidth or the right clock */
		if (i == ARRAY_SIZE(coeff_lut))
			return -EINVAL;

		ret = af9013_wr_regs(state, 0xae00, coeff_lut[i].val,
			sizeof(coeff_lut[i].val));
	}

	/* program frequency control */
	if (c->bandwidth_hz != state->bandwidth_hz || state->first_tune) {
		/* get used IF frequency */
		if (fe->ops.tuner_ops.get_if_frequency)
			fe->ops.tuner_ops.get_if_frequency(fe, &if_frequency);
		else
			if_frequency = state->config.if_frequency;

		dev_dbg(&state->i2c->dev, "%s: if_frequency=%d\n",
				__func__, if_frequency);

		sampling_freq = if_frequency;

		while (sampling_freq > (state->config.clock / 2))
			sampling_freq -= state->config.clock;

		if (sampling_freq < 0) {
			sampling_freq *= -1;
			spec_inv = state->config.spec_inv;
		} else {
			spec_inv = !state->config.spec_inv;
		}

		freq_cw = af9013_div(state, sampling_freq, state->config.clock,
				23);

		if (spec_inv)
			freq_cw = 0x800000 - freq_cw;

		buf[0] = (freq_cw >>  0) & 0xff;
		buf[1] = (freq_cw >>  8) & 0xff;
		buf[2] = (freq_cw >> 16) & 0x7f;

		freq_cw = 0x800000 - freq_cw;

		buf[3] = (freq_cw >>  0) & 0xff;
		buf[4] = (freq_cw >>  8) & 0xff;
		buf[5] = (freq_cw >> 16) & 0x7f;

		ret = af9013_wr_regs(state, 0xd140, buf, 3);
		if (ret)
			goto err;

		ret = af9013_wr_regs(state, 0x9be7, buf, 6);
		if (ret)
			goto err;
	}

	/* clear TPS lock flag */
	ret = af9013_wr_reg_bits(state, 0xd330, 3, 1, 1);
	if (ret)
		goto err;

	/* clear MPEG2 lock flag */
	ret = af9013_wr_reg_bits(state, 0xd507, 6, 1, 0);
	if (ret)
		goto err;

	/* empty channel function */
	ret = af9013_wr_reg_bits(state, 0x9bfe, 0, 1, 0);
	if (ret)
		goto err;

	/* empty DVB-T channel function */
	ret = af9013_wr_reg_bits(state, 0x9bc2, 0, 1, 0);
	if (ret)
		goto err;

	/* transmission parameters */
	auto_mode = false;
	memset(buf, 0, 3);

	switch (c->transmission_mode) {
	case TRANSMISSION_MODE_AUTO:
		auto_mode = true;
		break;
	case TRANSMISSION_MODE_2K:
		break;
	case TRANSMISSION_MODE_8K:
		buf[0] |= (1 << 0);
		break;
	default:
		dev_dbg(&state->i2c->dev, "%s: invalid transmission_mode\n",
				__func__);
		auto_mode = true;
	}

	switch (c->guard_interval) {
	case GUARD_INTERVAL_AUTO:
		auto_mode = true;
		break;
	case GUARD_INTERVAL_1_32:
		break;
	case GUARD_INTERVAL_1_16:
		buf[0] |= (1 << 2);
		break;
	case GUARD_INTERVAL_1_8:
		buf[0] |= (2 << 2);
		break;
	case GUARD_INTERVAL_1_4:
		buf[0] |= (3 << 2);
		break;
	default:
		dev_dbg(&state->i2c->dev, "%s: invalid guard_interval\n",
				__func__);
		auto_mode = true;
	}

	switch (c->hierarchy) {
	case HIERARCHY_AUTO:
		auto_mode = true;
		break;
	case HIERARCHY_NONE:
		break;
	case HIERARCHY_1:
		buf[0] |= (1 << 4);
		break;
	case HIERARCHY_2:
		buf[0] |= (2 << 4);
		break;
	case HIERARCHY_4:
		buf[0] |= (3 << 4);
		break;
	default:
		dev_dbg(&state->i2c->dev, "%s: invalid hierarchy\n", __func__);
		auto_mode = true;
	}

	switch (c->modulation) {
	case QAM_AUTO:
		auto_mode = true;
		break;
	case QPSK:
		break;
	case QAM_16:
		buf[1] |= (1 << 6);
		break;
	case QAM_64:
		buf[1] |= (2 << 6);
		break;
	default:
		dev_dbg(&state->i2c->dev, "%s: invalid modulation\n", __func__);
		auto_mode = true;
	}

	/* Use HP. How and which case we can switch to LP? */
	buf[1] |= (1 << 4);

	switch (c->code_rate_HP) {
	case FEC_AUTO:
		auto_mode = true;
		break;
	case FEC_1_2:
		break;
	case FEC_2_3:
		buf[2] |= (1 << 0);
		break;
	case FEC_3_4:
		buf[2] |= (2 << 0);
		break;
	case FEC_5_6:
		buf[2] |= (3 << 0);
		break;
	case FEC_7_8:
		buf[2] |= (4 << 0);
		break;
	default:
		dev_dbg(&state->i2c->dev, "%s: invalid code_rate_HP\n",
				__func__);
		auto_mode = true;
	}

	switch (c->code_rate_LP) {
	case FEC_AUTO:
		auto_mode = true;
		break;
	case FEC_1_2:
		break;
	case FEC_2_3:
		buf[2] |= (1 << 3);
		break;
	case FEC_3_4:
		buf[2] |= (2 << 3);
		break;
	case FEC_5_6:
		buf[2] |= (3 << 3);
		break;
	case FEC_7_8:
		buf[2] |= (4 << 3);
		break;
	case FEC_NONE:
		break;
	default:
		dev_dbg(&state->i2c->dev, "%s: invalid code_rate_LP\n",
				__func__);
		auto_mode = true;
	}

	switch (c->bandwidth_hz) {
	case 6000000:
		break;
	case 7000000:
		buf[1] |= (1 << 2);
		break;
	case 8000000:
		buf[1] |= (2 << 2);
		break;
	default:
		dev_dbg(&state->i2c->dev, "%s: invalid bandwidth_hz\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	ret = af9013_wr_regs(state, 0xd3c0, buf, 3);
	if (ret)
		goto err;

	if (auto_mode) {
		/* clear easy mode flag */
		ret = af9013_wr_reg(state, 0xaefd, 0);
		if (ret)
			goto err;

		dev_dbg(&state->i2c->dev, "%s: auto params\n", __func__);
	} else {
		/* set easy mode flag */
		ret = af9013_wr_reg(state, 0xaefd, 1);
		if (ret)
			goto err;

		ret = af9013_wr_reg(state, 0xaefe, 0);
		if (ret)
			goto err;

		dev_dbg(&state->i2c->dev, "%s: manual params\n", __func__);
	}

	/* tune */
	ret = af9013_wr_reg(state, 0xffff, 0);
	if (ret)
		goto err;

	state->bandwidth_hz = c->bandwidth_hz;
	state->set_frontend_jiffies = jiffies;
	state->first_tune = false;

	return ret;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
