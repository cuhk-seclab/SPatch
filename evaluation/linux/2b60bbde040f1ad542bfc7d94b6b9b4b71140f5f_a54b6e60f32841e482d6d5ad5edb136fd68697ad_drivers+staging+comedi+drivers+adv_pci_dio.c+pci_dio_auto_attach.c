static int pci_dio_auto_attach(struct comedi_device *dev,
			       unsigned long context)
{
	struct pci_dev *pcidev = comedi_to_pci_dev(dev);
	const struct dio_boardtype *board = NULL;
	const struct diosubd_data *d;
	struct comedi_subdevice *s;
	int ret, subdev, i, j;

	if (context < ARRAY_SIZE(boardtypes))
		board = &boardtypes[context];
	if (!board)
		return -ENODEV;
	dev->board_ptr = board;
	dev->board_name = board->name;

	ret = comedi_pci_enable(dev);
	if (ret)
		return ret;
	if (context == TYPE_PCI1736)
		dev->iobase = pci_resource_start(pcidev, 0);
	else
		dev->iobase = pci_resource_start(pcidev, 2);

	pci_dio_reset(dev, context);

	ret = comedi_alloc_subdevices(dev, board->nsubdevs);
	if (ret)
		return ret;

	subdev = 0;
	for (i = 0; i < MAX_DI_SUBDEVS; i++) {
		d = &board->sdi[i];
		if (d->chans) {
			s = &dev->subdevices[subdev++];
			s->type		= COMEDI_SUBD_DI;
			s->subdev_flags	= SDF_READABLE;
			s->n_chan	= d->chans;
			s->maxdata	= 1;
			s->range_table	= &range_digital;
			s->insn_bits	= board->is_16bit
						? pci_dio_insn_bits_di_w
						: pci_dio_insn_bits_di_b;
			s->private	= (void *)d->addr;
		}
	}

	for (i = 0; i < MAX_DO_SUBDEVS; i++) {
		d = &board->sdo[i];
		if (d->chans) {
			s = &dev->subdevices[subdev++];
			s->type		= COMEDI_SUBD_DO;
			s->subdev_flags	= SDF_WRITABLE;
			s->n_chan	= d->chans;
			s->maxdata	= 1;
			s->range_table	= &range_digital;
			s->insn_bits	= board->is_16bit
						? pci_dio_insn_bits_do_w
						: pci_dio_insn_bits_do_b;
			s->private	= (void *)d->addr;

			/* reset all outputs to 0 */
			if (board->is_16bit) {
				outw(0, dev->iobase + d->addr);
				if (s->n_chan > 16)
					outw(0, dev->iobase + d->addr + 2);
			} else {
				outb(0, dev->iobase + d->addr);
				if (s->n_chan > 8)
					outb(0, dev->iobase + d->addr + 1);
				if (s->n_chan > 16)
					outb(0, dev->iobase + d->addr + 2);
				if (s->n_chan > 24)
					outb(0, dev->iobase + d->addr + 3);
			}
		}
	}

	for (i = 0; i < MAX_DIO_SUBDEVG; i++) {
		d = &board->sdio[i];
		for (j = 0; j < d->chans; j++) {
			s = &dev->subdevices[subdev++];
			ret = subdev_8255_init(dev, s, NULL,
					       d->addr + j * I8255_SIZE);
			if (ret)
				return ret;
		}
	}

	if (board->id_reg) {
		s = &dev->subdevices[subdev++];
		s->type		= COMEDI_SUBD_DI;
		s->subdev_flags	= SDF_READABLE | SDF_INTERNAL;
		s->n_chan	= 4;
		s->maxdata	= 1;
		s->range_table	= &range_digital;
		s->insn_bits	= board->is_16bit ? pci_dio_insn_bits_di_w
						  : pci_dio_insn_bits_di_b;
		s->private	= (void *)board->id_reg;
	}

	if (board->timer_regbase) {
		s = &dev->subdevices[subdev++];

		dev->pacer = comedi_8254_init(dev->iobase +
					      board->timer_regbase,
					      0, I8254_IO8, 0);
		if (!dev->pacer)
			return -ENOMEM;

		comedi_8254_subdevice_init(s, dev->pacer);
	}

	return 0;
}
