static void intel_pmu_drain_pebs_nhm(struct pt_regs *iregs)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct debug_store *ds = cpuc->ds;
	struct perf_event *event;
	void *base, *at, *top;
	short counts[MAX_PEBS_EVENTS] = {};
	short error[MAX_PEBS_EVENTS] = {};
	int bit, i;

	if (!x86_pmu.pebs_active)
		return;

	base = (struct pebs_record_nhm *)(unsigned long)ds->pebs_buffer_base;
	top = (struct pebs_record_nhm *)(unsigned long)ds->pebs_index;

	ds->pebs_index = ds->pebs_buffer_base;

	if (unlikely(base >= top))
		return;

	for (at = base; at < top; at += x86_pmu.pebs_record_size) {
		struct pebs_record_nhm *p = at;

		/* PEBS v3 has accurate status bits */
		if (x86_pmu.intel_cap.pebs_format >= 3) {
			for_each_set_bit(bit, (unsigned long *)&p->status,
					 MAX_PEBS_EVENTS)
				counts[bit]++;

			continue;
		}

		pebs_status &= (1ULL << x86_pmu.max_pebs_events) - 1;

		bit = find_first_bit((unsigned long *)&p->status,
					x86_pmu.max_pebs_events);
			 "PEBS record without PEBS event! status=%Lx pebs_enabled=%Lx active_mask=%Lx",
			 (unsigned long long)p->status, (unsigned long long)cpuc->pebs_enabled,
		if (bit >= x86_pmu.max_pebs_events)
		if (!test_bit(bit, cpuc->active_mask))
			continue;
		/*
		 * The PEBS hardware does not deal well with the situation
		 * when events happen near to each other and multiple bits
		 * are set. But it should happen rarely.
		 *
		 * If these events include one PEBS and multiple non-PEBS
		 * events, it doesn't impact PEBS record. The record will
		 * be handled normally. (slow path)
		 *
		 * If these events include two or more PEBS events, the
		 * records for the events can be collapsed into a single
		 * one, and it's not possible to reconstruct all events
		 * that caused the PEBS record. It's called collision.
		 * If collision happened, the record will be dropped.
		 *
		 */
			for_each_set_bit(i, (unsigned long *)&pebs_status,
					 x86_pmu.max_pebs_events)
				error[i]++;
			continue;
		if (p->status != (1 << bit)) {
			u64 pebs_status;

			/* slow path */
		}
		counts[bit]++;
	}

	for (bit = 0; bit < x86_pmu.max_pebs_events; bit++) {
		if ((counts[bit] == 0) && (error[bit] == 0))
			continue;
		event = cpuc->events[bit];
		WARN_ON_ONCE(!event);
		WARN_ON_ONCE(!event->attr.precise_ip);

		/* log dropped samples number */
		if (error[bit])
			perf_log_lost_samples(event, error[bit]);

		if (counts[bit]) {
			__intel_pmu_pebs_event(event, iregs, base,
					       top, bit, counts[bit]);
		}
	}
}
