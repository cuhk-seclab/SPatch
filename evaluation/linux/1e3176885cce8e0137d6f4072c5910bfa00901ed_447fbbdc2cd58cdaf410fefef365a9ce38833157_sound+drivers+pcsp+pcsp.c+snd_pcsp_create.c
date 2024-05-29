static int snd_pcsp_create(struct snd_card *card)
{
	static struct snd_device_ops ops = { };
	unsigned int resolution = hrtimer_resolution;
	int err, div, min_div, order;

	if (!nopcm) {
		if (resolution > PCSP_MAX_PERIOD_NS) {
			printk(KERN_ERR "PCSP: Timer resolution is not sufficient "
				"(%linS)\n", resolution);
			printk(KERN_ERR "PCSP: Make sure you have HPET and ACPI "
				"enabled.\n");
			printk(KERN_ERR "PCSP: Turned into nopcm mode.\n");
			nopcm = 1;
		}
	}

	if (loops_per_jiffy >= PCSP_MIN_LPJ && resolution <= PCSP_MIN_PERIOD_NS)
		min_div = MIN_DIV;
	else
		min_div = MAX_DIV;
#if PCSP_DEBUG
	printk(KERN_DEBUG "PCSP: lpj=%li, min_div=%i, res=%li\n",
	       loops_per_jiffy, min_div, resolution);
#endif

	div = MAX_DIV / min_div;
	order = fls(div) - 1;

	pcsp_chip.max_treble = min(order, PCSP_MAX_TREBLE);
	pcsp_chip.treble = min(pcsp_chip.max_treble, PCSP_DEFAULT_TREBLE);
	pcsp_chip.playback_ptr = 0;
	pcsp_chip.period_ptr = 0;
	atomic_set(&pcsp_chip.timer_active, 0);
	pcsp_chip.enable = 1;
	pcsp_chip.pcspkr = 1;

	spin_lock_init(&pcsp_chip.substream_lock);

	pcsp_chip.card = card;
	pcsp_chip.port = 0x61;
	pcsp_chip.irq = -1;
	pcsp_chip.dma = -1;

	/* Register device */
	err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, &pcsp_chip, &ops);
	if (err < 0)
		return err;

	return 0;
}
