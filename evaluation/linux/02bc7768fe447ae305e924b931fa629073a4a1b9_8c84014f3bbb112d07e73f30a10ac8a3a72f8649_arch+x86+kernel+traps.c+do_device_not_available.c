dotraplinkage void
do_device_not_available(struct pt_regs *regs, long error_code)
{
	CT_WARN_ON(ct_state() != CONTEXT_KERNEL);
	BUG_ON(use_eager_fpu());

#ifdef CONFIG_MATH_EMULATION
	if (read_cr0() & X86_CR0_EM) {
		struct math_emu_info info = { };

		conditional_sti(regs);

		info.regs = regs;
		math_emulate(&info);
		return;
	}
#endif
	fpu__restore(&current->thread.fpu); /* interrupts still off */
#ifdef CONFIG_X86_32
	conditional_sti(regs);
#endif
}
