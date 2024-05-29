static void moom_callback(struct work_struct *ignored)
{
	const gfp_t gfp_mask = GFP_KERNEL;
	struct oom_control oc = {
		.zonelist = node_zonelist(first_memory_node, gfp_mask),
		.nodemask = NULL,
		.gfp_mask = gfp_mask,
		.order = 0,
		.force_kill = true,
	};

	mutex_lock(&oom_lock);
	if (!out_of_memory(&oc))
		pr_info("OOM request ignored because killer is disabled\n");
	mutex_unlock(&oom_lock);
}
