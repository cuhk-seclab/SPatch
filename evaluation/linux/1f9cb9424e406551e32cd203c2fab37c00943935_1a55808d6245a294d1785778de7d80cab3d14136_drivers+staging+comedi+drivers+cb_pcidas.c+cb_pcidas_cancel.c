static int cb_pcidas_cancel(struct comedi_device *dev,
			    struct comedi_subdevice *s)
{
	struct cb_pcidas_private *devpriv = dev->private;
	unsigned long flags;

	spin_lock_irqsave(&dev->spinlock, flags);
	/*  disable interrupts */
	devpriv->ctrl &= ~(PCIDAS_CTRL_INTE | PCIDAS_CTRL_EOAIE);
	outw(devpriv->ctrl, devpriv->pcibar1 + PCIDAS_CTRL_REG);
	spin_unlock_irqrestore(&dev->spinlock, flags);

	/*  disable start trigger source and burst mode */
	outw(PCIDAS_TRIG_SEL_NONE, devpriv->pcibar1 + PCIDAS_TRIG_REG);
	outw(PCIDAS_AI_PACER_SW, devpriv->pcibar1 + PCIDAS_AI_REG);

	return 0;
}
