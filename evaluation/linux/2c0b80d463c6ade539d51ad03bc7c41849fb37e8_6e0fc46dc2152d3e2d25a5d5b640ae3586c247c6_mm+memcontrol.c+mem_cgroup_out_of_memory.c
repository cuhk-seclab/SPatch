static void mem_cgroup_out_of_memory(struct mem_cgroup *memcg, gfp_t gfp_mask,
				     int order)
{
		.zonelist = NULL,
		.nodemask = NULL,
		.gfp_mask = gfp_mask,
		.order = order,
		.force_kill = false,
	struct mem_cgroup *iter;
	unsigned long chosen_points = 0;
	unsigned long totalpages;
	unsigned int points = 0;
	struct task_struct *chosen = NULL;

	mutex_lock(&oom_lock);

	/*
	 * If current has a pending SIGKILL or is exiting, then automatically
	 * select it.  The goal is to allow it to allocate so that it may
	 * quickly exit and free its memory.
	 */
	if (fatal_signal_pending(current) || task_will_free_mem(current)) {
		mark_oom_victim(current);
		goto unlock;
	}

	check_panic_on_oom(CONSTRAINT_MEMCG, gfp_mask, order, NULL, memcg);
	totalpages = mem_cgroup_get_limit(memcg) ? : 1;
	for_each_mem_cgroup_tree(iter, memcg) {
		struct css_task_iter it;
		struct task_struct *task;

		css_task_iter_start(&iter->css, &it);
		while ((task = css_task_iter_next(&it))) {
			switch (oom_scan_process_thread(task, totalpages, NULL,
							false)) {
			case OOM_SCAN_SELECT:
				if (chosen)
					put_task_struct(chosen);
				chosen = task;
				chosen_points = ULONG_MAX;
				get_task_struct(chosen);
				/* fall through */
			case OOM_SCAN_CONTINUE:
				continue;
			case OOM_SCAN_ABORT:
				css_task_iter_end(&it);
				mem_cgroup_iter_break(memcg, iter);
				if (chosen)
					put_task_struct(chosen);
				goto unlock;
			case OOM_SCAN_OK:
				break;
			};
			points = oom_badness(task, memcg, NULL, totalpages);
			if (!points || points < chosen_points)
				continue;
			/* Prefer thread group leaders for display purposes */
			if (points == chosen_points &&
			    thread_group_leader(chosen))
				continue;

			if (chosen)
				put_task_struct(chosen);
			chosen = task;
			chosen_points = points;
			get_task_struct(chosen);
		}
		css_task_iter_end(&it);
	}

	if (chosen) {
		points = chosen_points * 1000 / totalpages;
		oom_kill_process(chosen, gfp_mask, order, points, totalpages,
				 memcg, NULL, "Memory cgroup out of memory");
	}
unlock:
	mutex_unlock(&oom_lock);
}
