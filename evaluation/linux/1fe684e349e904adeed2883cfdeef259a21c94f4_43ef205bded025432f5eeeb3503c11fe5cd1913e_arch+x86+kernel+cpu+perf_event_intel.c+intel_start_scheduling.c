static void
intel_start_scheduling(struct cpu_hw_events *cpuc)
{
	struct intel_excl_cntrs *excl_cntrs = cpuc->excl_cntrs;
	struct intel_excl_states *xl;
	int tid = cpuc->excl_thread_id;

	/*
	 * nothing needed if in group validation mode
	 */
	if (cpuc->is_fake || !is_ht_workaround_enabled())
		return;

	/*
	 * no exclusion needed
	 */
	if (WARN_ON_ONCE(!excl_cntrs))
		return;

	xl = &excl_cntrs->states[tid];

	xl->sched_started = true;
	/*
	 * lock shared state until we are done scheduling
	 * in stop_event_scheduling()
	 * makes scheduling appear as a transaction
	 */
	raw_spin_lock(&excl_cntrs->lock);
}
