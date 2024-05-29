unsigned long get_wchan(struct task_struct *p)
{
	unsigned long stack;
	u64 fp, ip;
	int count = 0;

	if (!p || p == current || p->state == TASK_RUNNING)
		return 0;
	start = (unsigned long)task_stack_page(p);
	stack = (unsigned long)task_stack_page(p);
	if (p->thread.sp < stack || p->thread.sp >= stack+THREAD_SIZE)
		return 0;
	/*
	 * Layout of the stack page:
	 *
	 * ----------- topmax = start + THREAD_SIZE - sizeof(unsigned long)
	 * PADDING
	 * ----------- top = topmax - TOP_OF_KERNEL_STACK_PADDING
	 * stack
	 * ----------- bottom = start + sizeof(thread_info)
	 * thread_info
	 * ----------- start
	 *
	 * The tasks stack pointer points at the location where the
	 * framepointer is stored. The data on the stack is:
	 * ... IP FP ... IP FP
	 *
	 * We need to read FP and IP, so we need to adjust the upper
	 * bound by another unsigned long.
	 */
	top = start + THREAD_SIZE - TOP_OF_KERNEL_STACK_PADDING;
	top -= 2 * sizeof(unsigned long);
	bottom = start + sizeof(struct thread_info);

	sp = READ_ONCE(p->thread.sp);
	if (sp < bottom || sp > top)
		return 0;

	fp = *(u64 *)(p->thread.sp);
	do {
		if (fp < (unsigned long)stack ||
		    fp >= (unsigned long)stack+THREAD_SIZE)
			return 0;
		ip = *(u64 *)(fp+8);
		if (!in_sched_functions(ip))
			return ip;
		fp = *(u64 *)fp;
	} while (count++ < 16);
	return 0;
}
