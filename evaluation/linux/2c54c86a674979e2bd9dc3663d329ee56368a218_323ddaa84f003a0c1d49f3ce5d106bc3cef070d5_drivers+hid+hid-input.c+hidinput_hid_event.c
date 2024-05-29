void hidinput_hid_event(struct hid_device *hid, struct hid_field *field, struct hid_usage *usage, __s32 value)
{
	struct input_dev *input;
	unsigned *quirks = &hid->quirks;

	if (!field->hidinput)
		return;

	input = field->hidinput->input;

	if (!usage->type)
		return;

	if (usage->hat_min < usage->hat_max || usage->hat_dir) {
		int hat_dir = usage->hat_dir;
		if (!hat_dir)
			hat_dir = (value - usage->hat_min) * 8 / (usage->hat_max - usage->hat_min + 1) + 1;
		if (hat_dir < 0 || hat_dir > 8) hat_dir = 0;
		input_event(input, usage->type, usage->code    , hid_hat_to_axis[hat_dir].x);
		input_event(input, usage->type, usage->code + 1, hid_hat_to_axis[hat_dir].y);
		return;
	}

	if (usage->hid == (HID_UP_DIGITIZER | 0x003c)) { /* Invert */
		*quirks = value ? (*quirks | HID_QUIRK_INVERT) : (*quirks & ~HID_QUIRK_INVERT);
		return;
	}

	if (usage->hid == (HID_UP_DIGITIZER | 0x0032)) { /* InRange */
		if (value) {
			input_event(input, usage->type, (*quirks & HID_QUIRK_INVERT) ? BTN_TOOL_RUBBER : usage->code, 1);
			return;
		}
		input_event(input, usage->type, usage->code, 0);
		input_event(input, usage->type, BTN_TOOL_RUBBER, 0);
		return;
	}

	if (usage->hid == (HID_UP_DIGITIZER | 0x0030) && (*quirks & HID_QUIRK_NOTOUCH)) { /* Pressure */
		int a = field->logical_minimum;
		int b = field->logical_maximum;
		input_event(input, EV_KEY, BTN_TOUCH, value > a + ((b - a) >> 3));
	}

	if (usage->hid == (HID_UP_PID | 0x83UL)) { /* Simultaneous Effects Max */
		dbg_hid("Maximum Effects - %d\n",value);
		return;
	}

	if (usage->hid == (HID_UP_PID | 0x7fUL)) {
		dbg_hid("PID Pool Report\n");
		return;
	}

	if ((usage->type == EV_KEY) && (usage->code == 0)) /* Key 0 is "unassigned", not KEY_UNKNOWN */
		return;

	if ((usage->type == EV_ABS) && (field->flags & HID_MAIN_ITEM_RELATIVE) &&
			(usage->code == ABS_VOLUME)) {
		int count = abs(value);
		int direction = value > 0 ? KEY_VOLUMEUP : KEY_VOLUMEDOWN;
		int i;

		for (i = 0; i < count; i++) {
			input_event(input, EV_KEY, direction, 1);
			input_sync(input);
			input_event(input, EV_KEY, direction, 0);
			input_sync(input);
		}
		return;
	}

	/*
	 * Ignore out-of-range values as per HID specification,
	 * section 5.10 and 6.2.25.
	 *
	 * The logical_minimum < logical_maximum check is done so that we
	 * don't unintentionally discard values sent by devices which
	 * don't specify logical min and max.
	 */
	if ((field->flags & HID_MAIN_ITEM_VARIABLE) &&
	    (field->logical_minimum < field->logical_maximum) &&
	    (value < field->logical_minimum ||
	     value > field->logical_maximum)) {
		dbg_hid("Ignoring out-of-range value %x\n", value);
		return;
	}

	/*
	 * Ignore reports for absolute data if the data didn't change. This is
	 * not only an optimization but also fixes 'dead' key reports. Some
	 * RollOver implementations for localized keys (like BACKSLASH/PIPE; HID
	 * 0x31 and 0x32) report multiple keys, even though a localized keyboard
	 * can only have one of them physically available. The 'dead' keys
	 * report constant 0. As all map to the same keycode, they'd confuse
	 * the input layer. If we filter the 'dead' keys on the HID level, we
	 * skip the keycode translation and only forward real events.
	 */
	if (!(field->flags & (HID_MAIN_ITEM_RELATIVE |
	                      HID_MAIN_ITEM_BUFFERED_BYTE)) &&
			      (field->flags & HID_MAIN_ITEM_VARIABLE) &&
	    usage->usage_index < field->maxusage &&
	    value == field->value[usage->usage_index])
		return;

	/* report the usage code as scancode if the key status has changed */
	if (usage->type == EV_KEY &&
	    (!test_bit(usage->code, input->key)) == value)
		input_event(input, EV_MSC, MSC_SCAN, usage->hid);

	input_event(input, usage->type, usage->code, value);

	if ((field->flags & HID_MAIN_ITEM_RELATIVE) && (usage->type == EV_KEY))
		input_event(input, usage->type, usage->code, 0);
}
