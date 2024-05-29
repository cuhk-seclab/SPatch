void mem_cgroup_print_oom_info(struct mem_cgroup *memcg, struct task_struct *p)
{
	/* oom_info_lock ensures that parallel ooms do not interleave */
	static DEFINE_MUTEX(oom_info_lock);
	struct mem_cgroup *iter;
	unsigned int i;

	mutex_lock(&oom_info_lock);
	rcu_read_lock();

	if (p) {
		pr_info("Task in ");
		pr_cont_cgroup_path(task_cgroup(p, memory_cgrp_id));
		pr_cont(" killed as a result of limit of ");
	} else {
		pr_info("Memory limit reached of cgroup ");
	}

	pr_cont_cgroup_path(memcg->css.cgroup);
	pr_cont("\n");

	rcu_read_unlock();

	pr_info("memory: usage %llukB, limit %llukB, failcnt %lu\n",
		K((u64)page_counter_read(&memcg->memory)),
		K((u64)memcg->memory.limit), memcg->memory.failcnt);
	pr_info("memory+swap: usage %llukB, limit %llukB, failcnt %lu\n",
		K((u64)page_counter_read(&memcg->memsw)),
		K((u64)memcg->memsw.limit), memcg->memsw.failcnt);
	pr_info("kmem: usage %llukB, limit %llukB, failcnt %lu\n",
		K((u64)page_counter_read(&memcg->kmem)),
		K((u64)memcg->kmem.limit), memcg->kmem.failcnt);

	for_each_mem_cgroup_tree(iter, memcg) {
		pr_info("Memory cgroup stats for ");
		pr_cont_cgroup_path(iter->css.cgroup);
		pr_cont(":");

		for (i = 0; i < MEM_CGROUP_STAT_NSTATS; i++) {
			if (i == MEM_CGROUP_STAT_SWAP && !do_swap_account)
				continue;
			pr_cont(" %s:%ldKB", mem_cgroup_stat_names[i],
				K(mem_cgroup_read_stat(iter, i)));
		}

		for (i = 0; i < NR_LRU_LISTS; i++)
			pr_cont(" %s:%luKB", mem_cgroup_lru_names[i],
				K(mem_cgroup_nr_lru_pages(iter, BIT(i))));

		pr_cont("\n");
	}
	mutex_unlock(&oom_info_lock);
}
