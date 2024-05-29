static int pci1710_auto_attach(struct comedi_device *dev,
			       unsigned long context)
{
	struct pci_dev *pcidev = comedi_to_pci_dev(dev);
	const struct boardtype *board = NULL;
	struct pci1710_private *devpriv;
	struct comedi_subdevice *s;
	int ret, subdev, n_subdevices;
	int i;

	if (context < ARRAY_SIZE(boardtypes))
		board = &boardtypes[context];
	if (!board)
		return -ENODEV;
	dev->board_ptr = board;
	dev->board_name = board->name;

	devpriv = comedi_alloc_devpriv(dev, sizeof(*devpriv));
	if (!devpriv)
		return -ENOMEM;

	ret = comedi_pci_enable(dev);
	if (ret)
		return ret;
	dev->iobase = pci_resource_start(pcidev, 2);

	dev->pacer = comedi_8254_init(dev->iobase + PCI171X_TIMER_BASE,
				      I8254_OSC_BASE_10MHZ, I8254_IO16, 0);
	if (!dev->pacer)
		return -ENOMEM;

	n_subdevices = 1;	/* all boards have analog inputs */
	if (board->has_ao)
		n_subdevices++;
	if (!board->is_pci1713) {
		/*
		 * All other boards have digital inputs and outputs as
		 * well as a user counter.
		 */
		n_subdevices += 3;
	}

	ret = comedi_alloc_subdevices(dev, n_subdevices);
	if (ret)
		return ret;

	pci1710_reset(dev);

	if (pcidev->irq) {
		ret = request_irq(pcidev->irq, pci1710_irq_handler,
				  IRQF_SHARED, dev->board_name, dev);
		if (ret == 0)
			dev->irq = pcidev->irq;
	}

	subdev = 0;

	/* Analog Input subdevice */
	s = &dev->subdevices[subdev];
	s->type		= COMEDI_SUBD_AI;
	s->subdev_flags	= SDF_READABLE | SDF_GROUND;
	if (!board->is_pci1711)
		s->subdev_flags	|= SDF_DIFF;
	s->n_chan	= board->is_pci1713 ? 32 : 16;
	s->maxdata	= 0x0fff;
	s->range_table	= board->ai_range;
	s->insn_read	= pci171x_ai_insn_read;
	if (dev->irq) {
		dev->read_subdev = s;
		s->subdev_flags	|= SDF_CMD_READ;
		s->len_chanlist	= s->n_chan;
		s->do_cmdtest	= pci171x_ai_cmdtest;
		s->do_cmd	= pci171x_ai_cmd;
		s->cancel	= pci171x_ai_cancel;
	}

	/* find the value needed to adjust for unipolar gain codes */
	for (i = 0; i < s->range_table->length; i++) {
		if (comedi_range_is_unipolar(s, i)) {
			devpriv->unipolar_gain = i;
			break;
		}
	}

	subdev++;

	if (board->has_ao) {
		/* Analog Output subdevice */
		s = &dev->subdevices[subdev];
		s->type		= COMEDI_SUBD_AO;
		s->subdev_flags	= SDF_WRITABLE | SDF_GROUND;
		s->n_chan	= 2;
		s->maxdata	= 0x0fff;
		s->range_table	= &pci171x_ao_range;
		s->insn_write	= pci1710_ao_insn_write;

		ret = comedi_alloc_subdev_readback(s);
		if (ret)
			return ret;

		subdev++;
	}

	if (!board->is_pci1713) {
		/* Digital Input subdevice */
		s = &dev->subdevices[subdev];
		s->type		= COMEDI_SUBD_DI;
		s->subdev_flags	= SDF_READABLE;
		s->n_chan	= 16;
		s->maxdata	= 1;
		s->range_table	= &range_digital;
		s->insn_bits	= pci1710_di_insn_bits;
		subdev++;

		/* Digital Output subdevice */
		s = &dev->subdevices[subdev];
		s->type		= COMEDI_SUBD_DO;
		s->subdev_flags	= SDF_WRITABLE;
		s->n_chan	= 16;
		s->maxdata	= 1;
		s->range_table	= &range_digital;
		s->insn_bits	= pci1710_do_insn_bits;
		subdev++;

		/* Counter subdevice (8254) */
		s = &dev->subdevices[subdev];
		comedi_8254_subdevice_init(s, dev->pacer);

		dev->pacer->insn_config = pci1710_counter_insn_config;

		/* counters 1 and 2 are used internally for the pacer */
		comedi_8254_set_busy(dev->pacer, 1, true);
		comedi_8254_set_busy(dev->pacer, 2, true);

		subdev++;
	}

	/* max_samples is half the FIFO size (2 bytes/sample) */
	devpriv->max_samples = (board->is_pci1711) ? 512 : 2048;

	return 0;
}
