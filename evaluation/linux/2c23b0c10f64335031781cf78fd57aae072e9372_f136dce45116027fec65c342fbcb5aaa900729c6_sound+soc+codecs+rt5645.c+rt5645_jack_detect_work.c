static void rt5645_jack_detect_work(struct work_struct *work)
{
	struct rt5645_priv *rt5645 =
		container_of(work, struct rt5645_priv, jack_detect_work.work);
	int val, btn_type, gpio_state = 0, report = 0;

	if (!rt5645->codec)
		return;

	switch (rt5645->pdata.jd_mode) {
	case 0: /* Not using rt5645 JD */
		if (rt5645->gpiod_hp_det) {
			gpio_state = gpiod_get_value(rt5645->gpiod_hp_det);
			dev_dbg(rt5645->codec->dev, "gpio_state = %d\n",
				gpio_state);
			report = rt5645_jack_detect(rt5645->codec, gpio_state);
		}
		snd_soc_jack_report(rt5645->hp_jack,
				    report, SND_JACK_HEADPHONE);
		snd_soc_jack_report(rt5645->mic_jack,
				    report, SND_JACK_MICROPHONE);
		return;
	case 1: /* 2 port */
		val = snd_soc_read(rt5645->codec, RT5645_A_JD_CTRL1) & 0x0070;
		break;
	default: /* 1 port */
		val = snd_soc_read(rt5645->codec, RT5645_A_JD_CTRL1) & 0x0020;
		break;

	}

	switch (val) {
	/* jack in */
	case 0x30: /* 2 port */
	case 0x0: /* 1 port or 2 port */
		if (rt5645->jack_type == 0) {
			report = rt5645_jack_detect(rt5645->codec, 1);
			/* for push button and jack out */
			break;
		}
		btn_type = 0;
		if (snd_soc_read(rt5645->codec, RT5645_INT_IRQ_ST) & 0x4) {
			/* button pressed */
			report = SND_JACK_HEADSET;
			btn_type = rt5645_button_detect(rt5645->codec);
			/* rt5650 can report three kinds of button behavior,
			   one click, double click and hold. However,
			   currently we will report button pressed/released
			   event. So all the three button behaviors are
			   treated as button pressed. */
			switch (btn_type) {
			case 0x8000:
			case 0x4000:
			case 0x2000:
				report |= SND_JACK_BTN_0;
				break;
			case 0x1000:
			case 0x0800:
			case 0x0400:
				report |= SND_JACK_BTN_1;
				break;
			case 0x0200:
			case 0x0100:
			case 0x0080:
				report |= SND_JACK_BTN_2;
				break;
			case 0x0040:
			case 0x0020:
			case 0x0010:
				report |= SND_JACK_BTN_3;
				break;
			case 0x0000: /* unpressed */
				break;
			default:
				dev_err(rt5645->codec->dev,
					"Unexpected button code 0x%04x\n",
					btn_type);
				break;
			}
		}
		if (btn_type == 0)/* button release */
			report =  rt5645->jack_type;

		break;
	/* jack out */
	case 0x70: /* 2 port */
	case 0x10: /* 2 port */
	case 0x20: /* 1 port */
		report = 0;
		snd_soc_update_bits(rt5645->codec,
				    RT5645_INT_IRQ_ST, 0x1, 0x0);
		rt5645_jack_detect(rt5645->codec, 0);
		break;
	default:
		break;
	}

	snd_soc_jack_report(rt5645->hp_jack, report, SND_JACK_HEADPHONE);
	snd_soc_jack_report(rt5645->mic_jack, report, SND_JACK_MICROPHONE);
	if (rt5645->en_button_func)
		snd_soc_jack_report(rt5645->btn_jack,
			report, SND_JACK_BTN_0 | SND_JACK_BTN_1 |
				SND_JACK_BTN_2 | SND_JACK_BTN_3);
}
