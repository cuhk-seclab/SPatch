static int synchronize_sched_expedited_cpu_stop(void *data)
{
	struct rcu_data *rdp = data;
	struct rcu_state *rsp = rdp->rsp;

	/* We are here: If we are last, do the wakeup. */
	rdp->exp_done = true;
	if (atomic_dec_and_test(&rsp->expedited_need_qs))
		wake_up(&rsp->expedited_wq);
	return 0;
}
