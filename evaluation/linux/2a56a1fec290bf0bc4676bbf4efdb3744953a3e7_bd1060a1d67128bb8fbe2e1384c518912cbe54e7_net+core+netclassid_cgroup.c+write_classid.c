static int write_classid(struct cgroup_subsys_state *css, struct cftype *cft,
			 u64 value)
{
	struct cgroup_cls_state *cs = css_cls_state(css);

	cgroup_sk_alloc_disable();

	cs->classid = (u32)value;

	update_classid(css, (void *)(unsigned long)cs->classid);
	return 0;
}
