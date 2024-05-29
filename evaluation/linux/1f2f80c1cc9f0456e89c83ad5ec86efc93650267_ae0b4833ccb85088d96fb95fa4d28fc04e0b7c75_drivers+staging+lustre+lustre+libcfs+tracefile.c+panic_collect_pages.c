static void
panic_collect_pages(struct page_collection *pc)
{
	/* Do the collect_pages job on a single CPU: assumes that all other
	 * CPUs have been stopped during a panic.  If this isn't true for some
	 * arch, this will have to be implemented separately in each arch.  */
	int			i;
	int			j;
	struct cfs_trace_cpu_data *tcd;

	INIT_LIST_HEAD(&pc->pc_pages);

	cfs_tcd_for_each(tcd, i, j) {
		list_splice_init(&tcd->tcd_pages, &pc->pc_pages);
		tcd->tcd_cur_pages = 0;

		if (pc->pc_want_daemon_pages) {
			list_splice_init(&tcd->tcd_daemon_pages, &pc->pc_pages);
			tcd->tcd_cur_daemon_pages = 0;
		}
	}
}
