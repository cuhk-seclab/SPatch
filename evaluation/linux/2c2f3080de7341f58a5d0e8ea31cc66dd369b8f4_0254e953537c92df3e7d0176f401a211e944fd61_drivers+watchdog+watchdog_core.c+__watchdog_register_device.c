static int __watchdog_register_device(struct watchdog_device *wdd)
{
	int ret, id = -1;

	if (wdd == NULL || wdd->info == NULL || wdd->ops == NULL)
		return -EINVAL;

	/* Mandatory operations need to be supported */
	if (wdd->ops->start == NULL || wdd->ops->stop == NULL)
		return -EINVAL;

	watchdog_check_min_max_timeout(wdd);

	/*
	 * Note: now that all watchdog_device data has been verified, we
	 * will not check this anymore in other functions. If data gets
	 * corrupted in a later stage then we expect a kernel panic!
	 */

	/* Use alias for watchdog id if possible */
	if (wdd->parent) {
		ret = of_alias_get_id(wdd->parent->of_node, "watchdog");
		if (ret >= 0)
			id = ida_simple_get(&watchdog_ida, ret,
					    ret + 1, GFP_KERNEL);
	}

	if (id < 0)
		id = ida_simple_get(&watchdog_ida, 0, MAX_DOGS, GFP_KERNEL);

	if (id < 0)
		return id;
	wdd->id = id;

	ret = watchdog_dev_register(wdd);
	if (ret) {
		ida_simple_remove(&watchdog_ida, id);
		if (!(id == 0 && ret == -EBUSY))
			return ret;

		/* Retry in case a legacy watchdog module exists */
		id = ida_simple_get(&watchdog_ida, 1, MAX_DOGS, GFP_KERNEL);
		if (id < 0)
			return id;
		wdd->id = id;

		ret = watchdog_dev_register(wdd);
		if (ret) {
			ida_simple_remove(&watchdog_ida, id);
			return ret;
		}
	}

	if (test_bit(WDOG_STOP_ON_REBOOT, &wdd->status)) {
		wdd->reboot_nb.notifier_call = watchdog_reboot_notifier;

		ret = register_reboot_notifier(&wdd->reboot_nb);
		if (ret) {
			pr_err("watchdog%d: Cannot register reboot notifier (%d)\n",
			       wdd->id, ret);
			watchdog_dev_unregister(wdd);
			ida_simple_remove(&watchdog_ida, wdd->id);
			return ret;
		}
	}

	if (wdd->ops->restart) {
		wdd->restart_nb.notifier_call = watchdog_restart_notifier;

		ret = register_restart_handler(&wdd->restart_nb);
		if (ret)
			pr_warn("watchog%d: Cannot register restart handler (%d)\n",
				wdd->id, ret);
	}

	return 0;
}
