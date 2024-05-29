static int watchdog_start(struct watchdog_device *wdd)
{
	int err = 0;

	mutex_lock(&wdd->lock);

	if (test_bit(WDOG_UNREGISTERED, &wdd->status)) {
		err = -ENODEV;
		goto out_start;
	}

	if (watchdog_active(wdd))
		goto out_start;

	err = wdd->ops->start(wdd);
	if (err == 0)
		set_bit(WDOG_ACTIVE, &wdd->status);

out_start:
	mutex_unlock(&wdd->lock);
	return err;
}
