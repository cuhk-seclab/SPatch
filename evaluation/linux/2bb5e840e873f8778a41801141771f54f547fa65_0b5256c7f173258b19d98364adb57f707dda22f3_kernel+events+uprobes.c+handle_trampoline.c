static void handle_trampoline(struct pt_regs *regs)
{
	struct uprobe_task *utask;
	struct return_instance *ri;
	bool chained;

	utask = current->utask;
	if (!utask)
		goto sigill;

	ri = utask->return_instances;
	if (!ri)
		goto sigill;

	/*
	 * TODO: we should throw out return_instance's invalidated by
	 * longjmp(), currently we assume that the probed function always
	 * returns.
	 */
	instruction_pointer_set(regs, ri->orig_ret_vaddr);

	for (;;) {
		handle_uretprobe_chain(ri, regs);

		chained = ri->chained;
		ri = free_ret_instance(ri);
		utask->depth--;

		if (!chained)
			break;
		BUG_ON(!ri);
	}

	utask->return_instances = ri;
	return;

 sigill:
	uprobe_warn(current, "handle uretprobe, sending SIGILL.");
	force_sig_info(SIGILL, SEND_SIG_FORCED, current);

}
