static void print_cpu_stall(struct rcu_state *rsp)
{
	int cpu;
	unsigned long flags;
	struct rcu_node *rnp = rcu_get_root(rsp);
	long totqlen = 0;

	/*
	 * OK, time to rat on ourselves...
	 * See Documentation/RCU/stallwarn.txt for info on how to debug
	 * RCU CPU stall warnings.
	 */
	pr_err("INFO: %s self-detected stall on CPU", rsp->name);
	print_cpu_stall_info_begin();
	print_cpu_stall_info(rsp, smp_processor_id());
	print_cpu_stall_info_end();
	for_each_possible_cpu(cpu)
		totqlen += per_cpu_ptr(rsp->rda, cpu)->qlen;
	pr_cont(" (t=%lu jiffies g=%ld c=%ld q=%lu)\n",
		jiffies - rsp->gp_start,
		(long)rsp->gpnum, (long)rsp->completed, totqlen);

	rcu_check_gp_kthread_starvation(rsp);

	rcu_dump_cpu_stacks(rsp);

	raw_spin_lock_irqsave_rcu_node(rnp, flags);
	if (ULONG_CMP_GE(jiffies, READ_ONCE(rsp->jiffies_stall)))
		WRITE_ONCE(rsp->jiffies_stall,
			   jiffies + 3 * rcu_jiffies_till_stall_check() + 3);
	raw_spin_unlock_irqrestore(&rnp->lock, flags);

	/*
	 * Attempt to revive the RCU machinery by forcing a context switch.
	 *
	 * A context switch would normally allow the RCU state machine to make
	 * progress and it could be we're stuck in kernel space without context
	 * switches for an entirely unreasonable amount of time.
	 */
	resched_cpu(smp_processor_id());
}
