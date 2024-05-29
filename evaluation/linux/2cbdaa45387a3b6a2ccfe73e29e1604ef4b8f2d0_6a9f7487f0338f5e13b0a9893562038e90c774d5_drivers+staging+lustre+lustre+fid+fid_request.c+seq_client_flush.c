void seq_client_flush(struct lu_client_seq *seq)
{
	wait_queue_t link;

	LASSERT(seq);
	init_waitqueue_entry(&link, current);
	mutex_lock(&seq->lcs_mutex);

	while (seq->lcs_update) {
		add_wait_queue(&seq->lcs_waitq, &link);
		set_current_state(TASK_UNINTERRUPTIBLE);
		mutex_unlock(&seq->lcs_mutex);

		schedule();

		mutex_lock(&seq->lcs_mutex);
		remove_wait_queue(&seq->lcs_waitq, &link);
		set_current_state(TASK_RUNNING);
	}

	fid_zero(&seq->lcs_fid);
	/**
	 * this id shld not be used for seq range allocation.
	 * set to -1 for dgb check.
	 */

	seq->lcs_space.lsr_index = -1;

	range_init(&seq->lcs_space);
	mutex_unlock(&seq->lcs_mutex);
}
