static int watchdog_release(struct inode *inode, struct file *file)
{
	struct watchdog_core_data *wd_data = file->private_data;
	struct watchdog_device *wdd;
	int err = -EBUSY;

	mutex_lock(&wd_data->lock);

	wdd = wd_data->wdd;
	if (!wdd)
		goto done;

	/*
	 * We only stop the watchdog if we received the magic character
	 * or if WDIOF_MAGICCLOSE is not set. If nowayout was set then
	 * watchdog_stop will fail.
	 */
	if (!test_bit(WDOG_ACTIVE, &wdd->status))
		err = 0;
	else if (test_and_clear_bit(_WDOG_ALLOW_RELEASE, &wd_data->status) ||
		 !(wdd->info->options & WDIOF_MAGICCLOSE))
		err = watchdog_stop(wdd);

	/* If the watchdog was not stopped, send a keepalive ping */
	if (err < 0) {
		pr_crit("watchdog%d: watchdog did not stop!\n", wdd->id);
		watchdog_ping(wdd);
	}

	/* make sure that /dev/watchdog can be re-opened */
	clear_bit(_WDOG_DEV_OPEN, &wd_data->status);

done:
	mutex_unlock(&wd_data->lock);
	/* Allow the owner module to be unloaded again */
	module_put(wd_data->cdev.owner);
	kref_put(&wd_data->kref, watchdog_core_data_release);
	return 0;
}
