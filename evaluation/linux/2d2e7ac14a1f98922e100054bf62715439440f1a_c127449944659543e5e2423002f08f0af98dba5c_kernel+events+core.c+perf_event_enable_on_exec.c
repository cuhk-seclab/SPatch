static void perf_event_enable_on_exec(int ctxn)
{
	struct perf_event_context *ctx, *clone_ctx = NULL;
	struct perf_event *event;
	unsigned long flags;
	int enabled = 0;
	int ret;

	local_irq_save(flags);
	ctx = current->perf_event_ctxp[ctxn];
	if (!ctx || !ctx->nr_events)
		goto out;

	/*
	 * We must ctxsw out cgroup events to avoid conflict
	 * when invoking perf_task_event_sched_in() later on
	 * in this function. Otherwise we end up trying to
	 * ctxswin cgroup events which are already scheduled
	 * in.
	 */
	perf_cgroup_sched_out(current, NULL);

	raw_spin_lock(&ctx->lock);
	task_ctx_sched_out(ctx);

	list_for_each_entry(event, &ctx->event_list, event_entry) {
		ret = event_enable_on_exec(event, ctx);
		if (ret)
			enabled = 1;
	}

	/*
	 * Unclone this context if we enabled any event.
	 */
	if (enabled)
		clone_ctx = unclone_ctx(ctx);

	raw_spin_unlock(&ctx->lock);

	/*
	 * Also calls ctxswin for cgroup events, if any:
	 */
	perf_event_context_sched_in(ctx, ctx->task);
out:
	local_irq_restore(flags);

	if (clone_ctx)
		put_ctx(clone_ctx);
}
