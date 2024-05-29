static void xen_set_cpu_features(struct cpuinfo_x86 *c)
{
	if (xen_pv_domain()) {
		clear_cpu_bug(c, X86_BUG_SYSRET_SS_ATTRS);
		set_cpu_cap(c, X86_FEATURE_XENPV);
	}
}
