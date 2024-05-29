static int apci1516_reset(struct comedi_device *dev)
{
	const struct apci1516_boardinfo *board = dev->board_ptr;
	struct apci1516_private *devpriv = dev->private;

	if (!board->has_wdog)
		return 0;

	outw(0x0, dev->iobase + APCI1516_DO_REG);

	addi_watchdog_reset(devpriv->wdog_iobase);

	return 0;
}
