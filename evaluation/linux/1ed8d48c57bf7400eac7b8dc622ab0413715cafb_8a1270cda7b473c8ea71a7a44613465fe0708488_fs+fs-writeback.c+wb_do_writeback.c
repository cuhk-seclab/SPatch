static long wb_do_writeback(struct bdi_writeback *wb)
{
	struct wb_writeback_work *work;
	long wrote = 0;

	set_bit(WB_writeback_running, &wb->state);
	while ((work = get_next_work_item(wb)) != NULL) {
		struct wb_completion *done = work->done;
		bool need_wake_up = false;

		trace_writeback_exec(wb->bdi, work);

		wrote += wb_writeback(wb, work);

		if (work->single_wait) {
		} else if (work->auto_free) {
			kfree(work);
		if (done && atomic_dec_and_test(&done->cnt))
			need_wake_up = true;
		if (need_wake_up)
			wake_up_all(&wb->bdi->wb_waitq);
	}

	/*
	 * Check for periodic writeback, kupdated() style
	 */
	wrote += wb_check_old_data_flush(wb);
	wrote += wb_check_background_flush(wb);
	clear_bit(WB_writeback_running, &wb->state);

	return wrote;
}
