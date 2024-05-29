static void dump_header(struct oom_control *oc, struct task_struct *p,
			struct mem_cgroup *memcg)
{
	pr_warning("%s invoked oom-killer: gfp_mask=0x%x, order=%d, "
		"oom_score_adj=%hd\n",
		current->comm, oc->gfp_mask, oc->order,
		current->signal->oom_score_adj);
	cpuset_print_current_mems_allowed();
	dump_stack();
	if (memcg)
		mem_cgroup_print_oom_info(memcg, p);
	else
		show_mem(SHOW_MEM_FILTER_NODES);
	if (sysctl_oom_dump_tasks)
		dump_tasks(memcg, oc->nodemask);
}
