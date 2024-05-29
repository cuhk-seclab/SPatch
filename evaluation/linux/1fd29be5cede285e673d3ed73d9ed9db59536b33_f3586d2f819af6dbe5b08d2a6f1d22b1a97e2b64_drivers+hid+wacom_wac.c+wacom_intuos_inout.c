static int wacom_intuos_inout(struct wacom_wac *wacom)
{
	struct wacom_features *features = &wacom->features;
	unsigned char *data = wacom->data;
	struct input_dev *input = wacom->input;
	int idx = 0;

	/* tool number */
	if (features->type == INTUOS)
		idx = data[1] & 0x01;

	/* Enter report */
	if ((data[1] & 0xfc) == 0xc0) {
		/* serial number of the tool */
		wacom->serial[idx] = ((data[3] & 0x0f) << 28) +
			(data[4] << 20) + (data[5] << 12) +
			(data[6] << 4) + (data[7] >> 4);

		wacom->id[idx] = (data[2] << 4) | (data[3] >> 4) |
			((data[7] & 0x0f) << 20) | ((data[8] & 0xf0) << 12);

		switch (wacom->id[idx]) {
		case 0x812: /* Inking pen */
		case 0x801: /* Intuos3 Inking pen */
		case 0x120802: /* Intuos4/5 Inking Pen */
		case 0x012:
			wacom->tool[idx] = BTN_TOOL_PENCIL;
			break;

		case 0x822: /* Pen */
		case 0x842:
		case 0x852:
		case 0x823: /* Intuos3 Grip Pen */
		case 0x813: /* Intuos3 Classic Pen */
		case 0x885: /* Intuos3 Marker Pen */
		case 0x802: /* Intuos4/5 13HD/24HD General Pen */
		case 0x804: /* Intuos4/5 13HD/24HD Marker Pen */
		case 0x022:
		case 0x100804: /* Intuos4/5 13HD/24HD Art Pen */
		case 0x140802: /* Intuos4/5 13HD/24HD Classic Pen */
		case 0x160802: /* Cintiq 13HD Pro Pen */
		case 0x180802: /* DTH2242 Pen */
		case 0x100802: /* Intuos4/5 13HD/24HD General Pen */
			wacom->tool[idx] = BTN_TOOL_PEN;
			break;

		case 0x832: /* Stroke pen */
		case 0x032:
			wacom->tool[idx] = BTN_TOOL_BRUSH;
			break;

		case 0x007: /* Mouse 4D and 2D */
		case 0x09c:
		case 0x094:
		case 0x017: /* Intuos3 2D Mouse */
		case 0x806: /* Intuos4 Mouse */
			wacom->tool[idx] = BTN_TOOL_MOUSE;
			break;

		case 0x096: /* Lens cursor */
		case 0x097: /* Intuos3 Lens cursor */
		case 0x006: /* Intuos4 Lens cursor */
			wacom->tool[idx] = BTN_TOOL_LENS;
			break;

		case 0x82a: /* Eraser */
		case 0x85a:
		case 0x91a:
		case 0xd1a:
		case 0x0fa:
		case 0x82b: /* Intuos3 Grip Pen Eraser */
		case 0x81b: /* Intuos3 Classic Pen Eraser */
		case 0x91b: /* Intuos3 Airbrush Eraser */
		case 0x80c: /* Intuos4/5 13HD/24HD Marker Pen Eraser */
		case 0x80a: /* Intuos4/5 13HD/24HD General Pen Eraser */
		case 0x90a: /* Intuos4/5 13HD/24HD Airbrush Eraser */
		case 0x14080a: /* Intuos4/5 13HD/24HD Classic Pen Eraser */
		case 0x10090a: /* Intuos4/5 13HD/24HD Airbrush Eraser */
		case 0x10080c: /* Intuos4/5 13HD/24HD Art Pen Eraser */
		case 0x16080a: /* Cintiq 13HD Pro Pen Eraser */
		case 0x18080a: /* DTH2242 Eraser */
		case 0x10080a: /* Intuos4/5 13HD/24HD General Pen Eraser */
			wacom->tool[idx] = BTN_TOOL_RUBBER;
			break;

		case 0xd12:
		case 0x912:
		case 0x112:
		case 0x913: /* Intuos3 Airbrush */
		case 0x902: /* Intuos4/5 13HD/24HD Airbrush */
		case 0x100902: /* Intuos4/5 13HD/24HD Airbrush */
			wacom->tool[idx] = BTN_TOOL_AIRBRUSH;
			break;

		default: /* Unknown tool */
			wacom->tool[idx] = BTN_TOOL_PEN;
			break;
		}
		return 1;
	}

	/*
	 * don't report events for invalid data
	 */
	/* older I4 styli don't work with new Cintiqs */
	if ((!((wacom->id[idx] >> 20) & 0x01) &&
			(features->type == WACOM_21UX2)) ||
	    /* Only large Intuos support Lense Cursor */
	    (wacom->tool[idx] == BTN_TOOL_LENS &&
		(features->type == INTUOS3 ||
		 features->type == INTUOS3S ||
		 features->type == INTUOS4 ||
		 features->type == INTUOS4S ||
		 features->type == INTUOS5 ||
		 features->type == INTUOS5S ||
		 features->type == INTUOSPM ||
		 features->type == INTUOSPS)) ||
	   /* Cintiq doesn't send data when RDY bit isn't set */
	   (features->type == CINTIQ && !(data[1] & 0x40)))
		return 1;

	wacom->shared->stylus_in_proximity = true;
	if (wacom->shared->touch_down)
		return 1;

	/* in Range while exiting */
	if (((data[1] & 0xfe) == 0x20) && wacom->reporting_data) {
		input_report_key(input, BTN_TOUCH, 0);
		input_report_abs(input, ABS_PRESSURE, 0);
		input_report_abs(input, ABS_DISTANCE, wacom->features.distance_max);
		return 2;
	}

	/* Exit report */
	if ((data[1] & 0xfe) == 0x80) {
		wacom->shared->stylus_in_proximity = false;
		wacom->reporting_data = false;

		/* don't report exit if we don't know the ID */
		if (!wacom->id[idx])
			return 1;

		/*
		 * Reset all states otherwise we lose the initial states
		 * when in-prox next time
		 */
		input_report_abs(input, ABS_X, 0);
		input_report_abs(input, ABS_Y, 0);
		input_report_abs(input, ABS_DISTANCE, 0);
		input_report_abs(input, ABS_TILT_X, 0);
		input_report_abs(input, ABS_TILT_Y, 0);
		if (wacom->tool[idx] >= BTN_TOOL_MOUSE) {
			input_report_key(input, BTN_LEFT, 0);
			input_report_key(input, BTN_MIDDLE, 0);
			input_report_key(input, BTN_RIGHT, 0);
			input_report_key(input, BTN_SIDE, 0);
			input_report_key(input, BTN_EXTRA, 0);
			input_report_abs(input, ABS_THROTTLE, 0);
			input_report_abs(input, ABS_RZ, 0);
		} else {
			input_report_abs(input, ABS_PRESSURE, 0);
			input_report_key(input, BTN_STYLUS, 0);
			input_report_key(input, BTN_STYLUS2, 0);
			input_report_key(input, BTN_TOUCH, 0);
			input_report_abs(input, ABS_WHEEL, 0);
			if (features->type >= INTUOS3S)
				input_report_abs(input, ABS_Z, 0);
		}
		input_report_key(input, wacom->tool[idx], 0);
		input_report_abs(input, ABS_MISC, 0); /* reset tool id */
		input_event(input, EV_MSC, MSC_SERIAL, wacom->serial[idx]);
		wacom->id[idx] = 0;
		return 2;
	}

	/* don't report other events if we don't know the ID */
	if (!wacom->id[idx]) {
		/* but reschedule a read of the current tool */
		wacom_intuos_schedule_prox_event(wacom);
		return 1;
	}

	return 0;
}
