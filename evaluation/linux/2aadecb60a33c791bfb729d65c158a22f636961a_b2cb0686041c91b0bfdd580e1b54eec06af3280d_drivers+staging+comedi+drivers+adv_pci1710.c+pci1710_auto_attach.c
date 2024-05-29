static int pci1710_auto_attach(struct comedi_device *dev,
			       unsigned long context)
{
	struct pci_dev *pcidev = comedi_to_pci_dev(dev);
	const struct boardtype *board = NULL;
	struct pci1710_private *devpriv;
	struct comedi_subdevice *s;
	int ret, subdev, n_subdevices;

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

	n_subdevices = 0;
	if (board->n_aichan)
		n_subdevices++;
	if (board->has_ao)
		n_subdevices++;
	if (board->has_di_do)
		n_subdevices += 2;
	if (board->has_counter)
		n_subdevices++;

	ret = comedi_alloc_subdevices(dev, n_subdevices);
	if (ret)
		return ret;

	pci1710_reset(dev);

	if (board->has_irq && pcidev->irq) {
		ret = request_irq(pcidev->irq, interrupt_service_pci1710,
				  IRQF_SHARED, dev->board_name, dev);
		if (ret == 0)
			dev->irq = pcidev->irq;
	}

	subdev = 0;

	if (board->n_aichan) {
		s = &dev->subdevices[subdev];
		s->type		= COMEDI_SUBD_AI;
		s->subdev_flags	= SDF_READABLE | SDF_COMMON | SDF_GROUND;
		if (board->has_diff_ai)
			s->subdev_flags	|= SDF_DIFF;
		s->n_chan	= board->n_aichan;
		s->maxdata	= 0x0fff;
		s->range_table	= board->rangelist_ai;
		s->insn_read	= pci171x_ai_insn_read;
		if (dev->irq) {
			dev->read_subdev = s;
			s->subdev_flags	|= SDF_CMD_READ;
			s->len_chanlist	= s->n_chan;
			s->do_cmdtest	= pci171x_ai_cmdtest;
			s->do_cmd	= pci171x_ai_cmd;
			s->cancel	= pci171x_ai_cancel;
		}
		subdev++;
	}

	if (board->has_ao) {
		s = &dev->subdevices[subdev];
		s->type		= COMEDI_SUBD_AO;
		s->subdev_flags	= SDF_WRITABLE | SDF_GROUND | SDF_COMMON;
		s->n_chan	= 2;
		s->maxdata	= 0x0fff;
		s->range_table	= &pci171x_ao_range;
		s->insn_write	= pci171x_ao_insn_write;

		ret = comedi_alloc_subdev_readback(s);
		if (ret)
			return ret;

		subdev++;
	}

	if (board->has_di_do) {
		s = &dev->subdevices[subdev];
		s->type		= COMEDI_SUBD_DI;
		s->subdev_flags	= SDF_READABLE;
		s->n_chan	= 16;
		s->maxdata	= 1;
		s->range_table	= &range_digital;
		s->insn_bits	= pci171x_di_insn_bits;
		subdev++;

		s = &dev->subdevices[subdev];
		s->type		= COMEDI_SUBD_DO;
		s->subdev_flags	= SDF_WRITABLE;
		s->n_chan	= 16;
		s->maxdata	= 1;
		s->range_table	= &range_digital;
		s->insn_bits	= pci171x_do_insn_bits;
		subdev++;
	}

	/* Counter subdevice (8254) */
	if (board->has_counter) {
		s = &dev->subdevices[subdev];
		comedi_8254_subdevice_init(s, dev->pacer);

		dev->pacer->insn_config = pci171x_insn_counter_config;

		/* counters 1 and 2 are used internally for the pacer */
		comedi_8254_set_busy(dev->pacer, 1, true);
		comedi_8254_set_busy(dev->pacer, 2, true);

		subdev++;
	}

	/* max_samples is half the FIFO size (2 bytes/sample) */
	devpriv->max_samples = (board->has_large_fifo) ? 2048 : 512;

	return 0;
}
