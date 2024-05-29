static int
xfsaild(
	void		*data)
{
	struct xfs_ail	*ailp = data;
	long		tout = 0;	/* milliseconds */

	current->flags |= PF_MEMALLOC;
	set_freezable();

	while (!kthread_should_stop()) {
		if (tout && tout <= 20)
			__set_current_state(TASK_KILLABLE);
		else
			__set_current_state(TASK_INTERRUPTIBLE);

		spin_lock(&ailp->xa_lock);

		/*
		 * Idle if the AIL is empty and we are not racing with a target
		 * update. We check the AIL after we set the task to a sleep
		 * state to guarantee that we either catch an xa_target update
		 * or that a wake_up resets the state to TASK_RUNNING.
		 * Otherwise, we run the risk of sleeping indefinitely.
		 *
		 * The barrier matches the xa_target update in xfs_ail_push().
		 */
		smp_rmb();
		if (!xfs_ail_min(ailp) &&
		    ailp->xa_target == ailp->xa_target_prev) {
			spin_unlock(&ailp->xa_lock);
			schedule();
			tout = 0;
			continue;
		}
		spin_unlock(&ailp->xa_lock);

		if (tout)
			schedule_timeout(msecs_to_jiffies(tout));

		__set_current_state(TASK_RUNNING);

		try_to_freeze();

		tout = xfsaild_push(ailp);
	}

	return 0;
}
