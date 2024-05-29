static short _rtl92e_init(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	memset(&(priv->stats), 0, sizeof(struct rt_stats));

	_rtl92e_init_priv_handler(dev);
	_rtl92e_init_priv_constant(dev);
	_rtl92e_init_priv_variable(dev);
	_rtl92e_init_priv_lock(priv);
	_rtl92e_init_priv_task(dev);
	priv->ops->get_eeprom_size(dev);
	priv->ops->init_adapter_variable(dev);
	_rtl92e_get_channel_map(dev);

	rtl92e_dm_init(dev);

	setup_timer(&priv->watch_dog_timer,
		    _rtl92e_watchdog_timer_cb,
		    (unsigned long) dev);

	setup_timer(&priv->gpio_polling_timer,
		    rtl92e_check_rfctrl_gpio_timer,
		    (unsigned long)dev);

	rtl92e_irq_disable(dev);
	if (request_irq(dev->irq, _rtl92e_irq, IRQF_SHARED, dev->name, dev)) {
		netdev_err(dev, "Error allocating IRQ %d", dev->irq);
		return -1;
	}

	priv->irq = dev->irq;
	RT_TRACE(COMP_INIT, "IRQ %d\n", dev->irq);

	if (_rtl92e_pci_initdescring(dev) != 0) {
		netdev_err(dev, "Endopoints initialization failed");
		free_irq(dev->irq, dev);
		return -1;
	}

	return 0;
}
