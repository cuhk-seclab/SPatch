static int waveform_attach(struct comedi_device *dev,
			   struct comedi_devconfig *it)
{
	struct waveform_private *devpriv;
	struct comedi_subdevice *s;
	int amplitude = it->options[0];
	int period = it->options[1];
	int i;
	int ret;

	devpriv = comedi_alloc_devpriv(dev, sizeof(*devpriv));
	if (!devpriv)
		return -ENOMEM;

	/* set default amplitude and period */
	if (amplitude <= 0)
		amplitude = 1000000;	/* 1 volt */
	if (period <= 0)
		period = 100000;	/* 0.1 sec */

	devpriv->wf_amplitude = amplitude;
	devpriv->wf_period = period;

	ret = comedi_alloc_subdevices(dev, 2);
	if (ret)
		return ret;

	s = &dev->subdevices[0];
	dev->read_subdev = s;
	/* analog input subdevice */
	s->type = COMEDI_SUBD_AI;
	s->subdev_flags = SDF_READABLE | SDF_GROUND | SDF_CMD_READ;
	s->n_chan = N_CHANS;
	s->maxdata = 0xffff;
	s->range_table = &waveform_ai_ranges;
	s->len_chanlist = s->n_chan * 2;
	s->insn_read = waveform_ai_insn_read;
	s->do_cmd = waveform_ai_cmd;
	s->do_cmdtest = waveform_ai_cmdtest;
	s->cancel = waveform_ai_cancel;

	s = &dev->subdevices[1];
	dev->write_subdev = s;
	/* analog output subdevice (loopback) */
	s->type = COMEDI_SUBD_AO;
	s->subdev_flags = SDF_WRITABLE | SDF_GROUND;
	s->n_chan = N_CHANS;
	s->maxdata = 0xffff;
	s->range_table = &waveform_ai_ranges;
	s->insn_write = waveform_ao_insn_write;
	s->insn_read = waveform_ai_insn_read;	/* do same as AI insn_read */

	/* Our default loopback value is just a 0V flatline */
	for (i = 0; i < s->n_chan; i++)
		devpriv->ao_loopbacks[i] = s->maxdata / 2;

	setup_timer(&devpriv->ai_timer, waveform_ai_timer, (unsigned long)dev);

	dev_info(dev->class_dev,
		 "%s: %u microvolt, %u microsecond waveform attached\n",
		 dev->board_name,
		 devpriv->wf_amplitude, devpriv->wf_period);

	return 0;
}
