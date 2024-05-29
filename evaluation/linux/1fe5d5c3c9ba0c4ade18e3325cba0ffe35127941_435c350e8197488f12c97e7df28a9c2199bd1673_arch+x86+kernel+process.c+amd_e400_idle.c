static void amd_e400_idle(void)
{
	if (!amd_e400_c1e_detected) {
		u32 lo, hi;

		rdmsr(MSR_K8_INT_PENDING_MSG, lo, hi);

		if (lo & K8_INTP_C1E_ACTIVE_MASK) {
			amd_e400_c1e_detected = true;
			if (!boot_cpu_has(X86_FEATURE_NONSTOP_TSC))
				mark_tsc_unstable("TSC halt in AMD C1E");
			pr_info("System has AMD C1E enabled\n");
		}
	}

	if (amd_e400_c1e_detected) {
		int cpu = smp_processor_id();

		if (!cpumask_test_cpu(cpu, amd_e400_c1e_mask)) {
			cpumask_set_cpu(cpu, amd_e400_c1e_mask);
			/* Force broadcast so ACPI can not interfere. */
			tick_broadcast_force();
			pr_info("Switch to broadcast mode on CPU%d\n", cpu);
		}
		tick_broadcast_enter();

		default_idle();

		/*
		 * The switch back from broadcast mode needs to be
		 * called with interrupts disabled.
		 */
		local_irq_disable();
		tick_broadcast_exit();
		local_irq_enable();
	} else
		default_idle();
}
