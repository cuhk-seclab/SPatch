static int dio24_auto_attach(struct comedi_device *dev,
			     unsigned long context)
{
	struct pcmcia_device *link = comedi_to_pcmcia_dev(dev);
	struct comedi_subdevice *s;
	int ret;

	link->config_flags |= CONF_AUTO_SET_IO;
	ret = comedi_pcmcia_enable(dev, NULL);
	if (ret)
		return ret;
	dev->iobase = link->resource[0]->start;

	ret = comedi_alloc_subdevices(dev, 1);
	if (ret)
		return ret;

	/* 8255 dio */
	s = &dev->subdevices[0];
	return subdev_8255_init(dev, s, NULL, 0x00);
}
