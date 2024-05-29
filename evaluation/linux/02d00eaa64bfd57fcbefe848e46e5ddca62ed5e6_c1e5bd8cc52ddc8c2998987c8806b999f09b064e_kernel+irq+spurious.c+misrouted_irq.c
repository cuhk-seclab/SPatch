static int misrouted_irq(int irq)
{
	struct irq_desc *desc;
	int i, ok = 0;

	if (atomic_inc_return(&irq_poll_active) != 1)
		goto out;

	irq_poll_cpu = smp_processor_id();

	for_each_irq_desc(i, desc) {
		if (!i)
			 continue;

		if (i == irq)	/* Already tried */
			continue;

		if (try_one_irq(desc, false))
			ok = 1;
	}
out:
	atomic_dec(&irq_poll_active);
	/* So the caller can adjust the irq error counts */
	return ok;
}
