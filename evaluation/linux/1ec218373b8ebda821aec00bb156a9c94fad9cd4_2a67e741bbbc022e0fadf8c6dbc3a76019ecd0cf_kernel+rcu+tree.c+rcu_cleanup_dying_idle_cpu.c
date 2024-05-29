static void rcu_cleanup_dying_idle_cpu(int cpu, struct rcu_state *rsp)
{
	unsigned long flags;
	unsigned long mask;
	struct rcu_data *rdp = per_cpu_ptr(rsp->rda, cpu);
	struct rcu_node *rnp = rdp->mynode;  /* Outgoing CPU's rdp & rnp. */

	if (!IS_ENABLED(CONFIG_HOTPLUG_CPU))
		return;

	/* Remove outgoing CPU from mask in the leaf rcu_node structure. */
	mask = rdp->grpmask;
	raw_spin_lock_irqsave_rcu_node(rnp, flags); /* Enforce GP memory-order guarantee. */
	rnp->qsmaskinitnext &= ~mask;
	raw_spin_unlock_irqrestore(&rnp->lock, flags);
}
