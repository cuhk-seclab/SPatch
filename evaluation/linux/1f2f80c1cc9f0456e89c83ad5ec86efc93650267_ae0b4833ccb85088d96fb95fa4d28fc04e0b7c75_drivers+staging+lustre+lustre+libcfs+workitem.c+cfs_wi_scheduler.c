static int
cfs_wi_scheduler (void *arg)
{
	struct cfs_wi_sched	*sched = (struct cfs_wi_sched *)arg;

	cfs_block_allsigs();

	/* CPT affinity scheduler? */
	if (sched->ws_cptab)
		if (cfs_cpt_bind(sched->ws_cptab, sched->ws_cpt) != 0)
			CWARN("Failed to bind %s on CPT %d\n",
			      sched->ws_name, sched->ws_cpt);

	spin_lock(&cfs_wi_data.wi_glock);

	LASSERT(sched->ws_starting == 1);
	sched->ws_starting--;
	sched->ws_nthreads++;

	spin_unlock(&cfs_wi_data.wi_glock);

	spin_lock(&sched->ws_lock);

	while (!sched->ws_stopping) {
		int	     nloops = 0;
		int	     rc;
		cfs_workitem_t *wi;

		while (!list_empty(&sched->ws_runq) &&
		       nloops < CFS_WI_RESCHED) {
			wi = list_entry(sched->ws_runq.next,
					    cfs_workitem_t, wi_list);
			LASSERT(wi->wi_scheduled && !wi->wi_running);

			list_del_init(&wi->wi_list);

			LASSERT(sched->ws_nscheduled > 0);
			sched->ws_nscheduled--;

			wi->wi_running   = 1;
			wi->wi_scheduled = 0;

			spin_unlock(&sched->ws_lock);
			nloops++;

			rc = (*wi->wi_action) (wi);

			spin_lock(&sched->ws_lock);
			if (rc != 0) /* WI should be dead, even be freed! */
				continue;

			wi->wi_running = 0;
			if (list_empty(&wi->wi_list))
				continue;

			LASSERT(wi->wi_scheduled);
			/* wi is rescheduled, should be on rerunq now, we
			 * move it to runq so it can run action now */
			list_move_tail(&wi->wi_list, &sched->ws_runq);
		}

		if (!list_empty(&sched->ws_runq)) {
			spin_unlock(&sched->ws_lock);
			/* don't sleep because some workitems still
			 * expect me to come back soon */
			cond_resched();
			spin_lock(&sched->ws_lock);
			continue;
		}

		spin_unlock(&sched->ws_lock);
		rc = wait_event_interruptible_exclusive(sched->ws_waitq,
						!cfs_wi_sched_cansleep(sched));
		spin_lock(&sched->ws_lock);
	}

	spin_unlock(&sched->ws_lock);

	spin_lock(&cfs_wi_data.wi_glock);
	sched->ws_nthreads--;
	spin_unlock(&cfs_wi_data.wi_glock);

	return 0;
}
