static int cgroup_update_dfl_csses(struct cgroup *cgrp)
{
	LIST_HEAD(preloaded_csets);
	struct cgroup_subsys_state *css;
	struct css_set *src_cset;
	int ret;

	lockdep_assert_held(&cgroup_mutex);

	/* look up all csses currently attached to @cgrp's subtree */
	down_read(&css_set_rwsem);
	css_for_each_descendant_pre(css, cgroup_css(cgrp, NULL)) {
		struct cgrp_cset_link *link;

		/* self is not affected by child_subsys_mask change */
		if (css->cgroup == cgrp)
			continue;

		list_for_each_entry(link, &css->cgroup->cset_links, cset_link)
			cgroup_migrate_add_src(link->cset, cgrp,
					       &preloaded_csets);
	}
	up_read(&css_set_rwsem);

	/* NULL dst indicates self on default hierarchy */
	ret = cgroup_migrate_prepare_dst(NULL, &preloaded_csets);
	if (ret)
		goto out_finish;

	list_for_each_entry(src_cset, &preloaded_csets, mg_preload_node) {
		struct task_struct *last_task = NULL, *task;

		/* src_csets precede dst_csets, break on the first dst_cset */
		if (!src_cset->mg_src_cgrp)
			break;

		/*
		 * All tasks in src_cset need to be migrated to the
		 * matching dst_cset.  Empty it process by process.  We
		 * walk tasks but migrate processes.  The leader might even
		 * belong to a different cset but such src_cset would also
		 * be among the target src_csets because the default
		 * hierarchy enforces per-process membership.
		 */
		while (true) {
			down_read(&css_set_rwsem);
			task = list_first_entry_or_null(&src_cset->tasks,
						struct task_struct, cg_list);
			if (task) {
				task = task->group_leader;
				WARN_ON_ONCE(!task_css_set(task)->mg_src_cgrp);
				get_task_struct(task);
			}
			up_read(&css_set_rwsem);

			if (!task)
				break;

			/* guard against possible infinite loop */
			if (WARN(last_task == task,
				 "cgroup: update_dfl_csses failed to make progress, aborting in inconsistent state\n"))
				goto out_finish;
			last_task = task;

			percpu_down_write(&cgroup_threadgroup_rwsem);

			ret = cgroup_migrate(src_cset->dfl_cgrp, task, true);

			percpu_up_write(&cgroup_threadgroup_rwsem);
			put_task_struct(task);

			if (WARN(ret, "cgroup: failed to update controllers for the default hierarchy (%d), further operations may crash or hang\n", ret))
				goto out_finish;
		}
	}

out_finish:
	cgroup_migrate_finish(&preloaded_csets);
	percpu_up_write(&cgroup_threadgroup_rwsem);
	return ret;
}
