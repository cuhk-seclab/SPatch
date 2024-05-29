void do_machine_check(struct pt_regs *regs, long error_code)
{
	struct mca_config *cfg = &mca_cfg;
	struct mce m, *final;
	int i;
	int worst = 0;
	int severity;
	/*
	 * Establish sequential order between the CPUs entering the machine
	 * check handler.
	 */
	int order;
	/*
	 * If no_way_out gets set, there is no safe way to recover from this
	 * MCE.  If mca_cfg.tolerant is cranked up, we'll try anyway.
	 */
	int no_way_out = 0;
	/*
	 * If kill_it gets set, there might be a way to recover from this
	 * error.
	 */
	int kill_it = 0;
	DECLARE_BITMAP(toclear, MAX_NR_BANKS);
	DECLARE_BITMAP(valid_banks, MAX_NR_BANKS);
	char *msg = "Unknown";
	u64 recover_paddr = ~0ull;
	int flags = MF_ACTION_REQUIRED;
	int lmce = 0;

	ist_enter(regs);

	this_cpu_inc(mce_exception_count);

	if (!cfg->banks)
		goto out;

	mce_gather_info(&m, regs);

	final = this_cpu_ptr(&mces_seen);
	*final = m;

	memset(valid_banks, 0, sizeof(valid_banks));
	no_way_out = mce_no_way_out(&m, &msg, valid_banks, regs);

	barrier();

	/*
	 * When no restart IP might need to kill or panic.
	 * Assume the worst for now, but if we find the
	 * severity is MCE_AR_SEVERITY we have other options.
	 */
	if (!(m.mcgstatus & MCG_STATUS_RIPV))
		kill_it = 1;

	/*
	 * Check if this MCE is signaled to only this logical processor
	 */
	if (m.mcgstatus & MCG_STATUS_LMCES)
		lmce = 1;
	else {
		/*
		 * Go through all the banks in exclusion of the other CPUs.
		 * This way we don't report duplicated events on shared banks
		 * because the first one to see it will clear it.
		 * If this is a Local MCE, then no need to perform rendezvous.
		 */
		order = mce_start(&no_way_out);
	}

	for (i = 0; i < cfg->banks; i++) {
		__clear_bit(i, toclear);
		if (!test_bit(i, valid_banks))
			continue;
		if (!mce_banks[i].ctl)
			continue;

		m.misc = 0;
		m.addr = 0;
		m.bank = i;

		m.status = mce_rdmsrl(MSR_IA32_MCx_STATUS(i));
		if ((m.status & MCI_STATUS_VAL) == 0)
			continue;

		/*
		 * Non uncorrected or non signaled errors are handled by
		 * machine_check_poll. Leave them alone, unless this panics.
		 */
		if (!(m.status & (cfg->ser ? MCI_STATUS_S : MCI_STATUS_UC)) &&
			!no_way_out)
			continue;

		/*
		 * Set taint even when machine check was not enabled.
		 */
		add_taint(TAINT_MACHINE_CHECK, LOCKDEP_NOW_UNRELIABLE);

		severity = mce_severity(&m, cfg->tolerant, NULL, true);

		/*
		 * When machine check was for corrected/deferred handler don't
		 * touch, unless we're panicing.
		 */
		if ((severity == MCE_KEEP_SEVERITY ||
		     severity == MCE_UCNA_SEVERITY) && !no_way_out)
			continue;
		__set_bit(i, toclear);
		if (severity == MCE_NO_SEVERITY) {
			/*
			 * Machine check event was not enabled. Clear, but
			 * ignore.
			 */
			continue;
		}

		mce_read_aux(&m, i);

		/*
		 * Action optional error. Queue address for later processing.
		 * When the ring overflows we just ignore the AO error.
		 * RED-PEN add some logging mechanism when
		 * usable_address or mce_add_ring fails.
		 * RED-PEN don't ignore overflow for mca_cfg.tolerant == 0
		 */
		if (severity == MCE_AO_SEVERITY && mce_usable_address(&m))
			mce_ring_add(m.addr >> PAGE_SHIFT);

		mce_log(&m);

		if (severity > worst) {
			*final = m;
			worst = severity;
		}
	}

	/* mce_clear_state will clear *final, save locally for use later */
	m = *final;

	if (!no_way_out)
		mce_clear_state(toclear);

	/*
	 * Do most of the synchronization with other CPUs.
	 * When there's any problem use only local no_way_out state.
	 */
	if (!lmce) {
		if (mce_end(order) < 0)
			no_way_out = worst >= MCE_PANIC_SEVERITY;
	} else {
		/*
		 * Local MCE skipped calling mce_reign()
		 * If we found a fatal error, we need to panic here.
		 */
		 if (worst >= MCE_PANIC_SEVERITY && mca_cfg.tolerant < 3)
			mce_panic("Machine check from unknown source",
				NULL, NULL);
	}

	/*
	 * At insane "tolerant" levels we take no action. Otherwise
	 * we only die if we have no other choice. For less serious
	 * issues we try to recover, or limit damage to the current
	 * process.
	 */
	if (cfg->tolerant < 3) {
		if (no_way_out)
			mce_panic("Fatal machine check on current CPU", &m, msg);
		if (worst == MCE_AR_SEVERITY) {
			recover_paddr = m.addr;
			if (!(m.mcgstatus & MCG_STATUS_RIPV))
				flags |= MF_MUST_KILL;
		} else if (kill_it) {
			force_sig(SIGBUS, current);
		}
	}

	if (worst > 0)
		mce_report_event(regs);
	mce_wrmsrl(MSR_IA32_MCG_STATUS, 0);
out:
	sync_core();

	if (recover_paddr == ~0ull)
		goto done;

	pr_err("Uncorrected hardware memory error in user-access at %llx",
		 recover_paddr);
	/*
	 * We must call memory_failure() here even if the current process is
	 * doomed. We still need to mark the page as poisoned and alert any
	 * other users of the page.
	 */
	ist_begin_non_atomic(regs);
	local_irq_enable();
	if (memory_failure(recover_paddr >> PAGE_SHIFT, MCE_VECTOR, flags) < 0) {
		pr_err("Memory error not recovered");
		force_sig(SIGBUS, current);
	}
	local_irq_disable();
	ist_end_non_atomic();
done:
	ist_exit(regs);
}
