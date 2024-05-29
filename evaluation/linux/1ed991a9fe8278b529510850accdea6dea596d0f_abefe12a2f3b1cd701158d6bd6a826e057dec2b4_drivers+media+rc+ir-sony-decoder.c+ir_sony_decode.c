static int ir_sony_decode(struct rc_dev *dev, struct ir_raw_event ev)
{
	struct sony_dec *data = &dev->raw->sony;
	enum rc_type protocol;
	u32 scancode;
	u8 device, subdevice, function;

	if (!(dev->enabled_protocols &
	      (RC_BIT_SONY12 | RC_BIT_SONY15 | RC_BIT_SONY20)))
		return 0;

	if (!is_timing_event(ev)) {
		if (ev.reset)
			data->state = STATE_INACTIVE;
		return 0;
	}

	if (!geq_margin(ev.duration, SONY_UNIT, SONY_UNIT / 2))
		goto out;

	IR_dprintk(2, "Sony decode started at state %d (%uus %s)\n",
		   data->state, TO_US(ev.duration), TO_STR(ev.pulse));

	switch (data->state) {

	case STATE_INACTIVE:
		if (!ev.pulse)
			break;

		if (!eq_margin(ev.duration, SONY_HEADER_PULSE, SONY_UNIT / 2))
			break;

		data->count = 0;
		data->state = STATE_HEADER_SPACE;
		return 0;

	case STATE_HEADER_SPACE:
		if (ev.pulse)
			break;

		if (!eq_margin(ev.duration, SONY_HEADER_SPACE, SONY_UNIT / 2))
			break;

		data->state = STATE_BIT_PULSE;
		return 0;

	case STATE_BIT_PULSE:
		if (!ev.pulse)
			break;

		data->bits <<= 1;
		if (eq_margin(ev.duration, SONY_BIT_1_PULSE, SONY_UNIT / 2))
			data->bits |= 1;
		else if (!eq_margin(ev.duration, SONY_BIT_0_PULSE, SONY_UNIT / 2))
			break;

		data->count++;
		data->state = STATE_BIT_SPACE;
		return 0;

	case STATE_BIT_SPACE:
		if (ev.pulse)
			break;

		if (!geq_margin(ev.duration, SONY_BIT_SPACE, SONY_UNIT / 2))
			break;

		decrease_duration(&ev, SONY_BIT_SPACE);

		if (!geq_margin(ev.duration, SONY_UNIT, SONY_UNIT / 2)) {
			data->state = STATE_BIT_PULSE;
			return 0;
		}

		data->state = STATE_FINISHED;
		/* Fall through */

	case STATE_FINISHED:
		if (ev.pulse)
			break;

		if (!geq_margin(ev.duration, SONY_TRAILER_SPACE, SONY_UNIT / 2))
			break;

		switch (data->count) {
		case 12:
			if (!(dev->enabled_protocols & RC_BIT_SONY12))
				goto finish_state_machine;

			device    = bitrev8((data->bits <<  3) & 0xF8);
			subdevice = 0;
			function  = bitrev8((data->bits >>  4) & 0xFE);
			protocol = RC_TYPE_SONY12;
			break;
		case 15:
			if (!(dev->enabled_protocols & RC_BIT_SONY15))
				goto finish_state_machine;

			device    = bitrev8((data->bits >>  0) & 0xFF);
			subdevice = 0;
			function  = bitrev8((data->bits >>  7) & 0xFE);
			protocol = RC_TYPE_SONY15;
			break;
		case 20:
			if (!(dev->enabled_protocols & RC_BIT_SONY20))
				goto finish_state_machine;

			device    = bitrev8((data->bits >>  5) & 0xF8);
			subdevice = bitrev8((data->bits >>  0) & 0xFF);
			function  = bitrev8((data->bits >> 12) & 0xFE);
			protocol = RC_TYPE_SONY20;
			break;
		default:
			IR_dprintk(1, "Sony invalid bitcount %u\n", data->count);
			goto out;
		}

		scancode = device << 16 | subdevice << 8 | function;
		IR_dprintk(1, "Sony(%u) scancode 0x%05x\n", data->count, scancode);
		rc_keydown(dev, protocol, scancode, 0);
		goto finish_state_machine;
	}

out:
	IR_dprintk(1, "Sony decode failed at state %d (%uus %s)\n",
		   data->state, TO_US(ev.duration), TO_STR(ev.pulse));
	data->state = STATE_INACTIVE;
	return -EINVAL;

finish_state_machine:
	data->state = STATE_INACTIVE;
	return 0;
}
