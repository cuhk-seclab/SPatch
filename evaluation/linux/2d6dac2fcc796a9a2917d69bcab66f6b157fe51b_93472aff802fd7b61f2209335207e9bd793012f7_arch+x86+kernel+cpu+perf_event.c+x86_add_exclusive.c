int x86_add_exclusive(unsigned int what)
{
	int i;

		mutex_lock(&pmc_reserve_mutex);
		for (i = 0; i < ARRAY_SIZE(x86_pmu.lbr_exclusive); i++) {
			if (i != what && atomic_read(&x86_pmu.lbr_exclusive[i]))
				goto fail_unlock;
		}
		atomic_inc(&x86_pmu.lbr_exclusive[what]);
	if (atomic_inc_not_zero(&x86_pmu.lbr_exclusive[what]))
			goto out;
	}

	atomic_inc(&active_events);
	return 0;

fail_unlock:
	mutex_unlock(&pmc_reserve_mutex);
	return -EBUSY;
}
