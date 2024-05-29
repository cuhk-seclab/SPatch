static bool reqsk_queue_unlink(struct request_sock_queue *queue,
			       struct request_sock *req)
{
	struct listen_sock *lopt = queue->listen_opt;
	struct request_sock **prev;
	bool found = false;

	spin_lock(&queue->syn_wait_lock);

	for (prev = &lopt->syn_table[req->rsk_hash]; *prev != NULL;
	     prev = &(*prev)->dl_next) {
		if (*prev == req) {
			*prev = req->dl_next;
			found = true;
			break;
		}
	}

	spin_unlock(&queue->syn_wait_lock);
	if (timer_pending(&req->rsk_timer) && del_timer_sync(&req->rsk_timer))
		reqsk_put(req);
	return found;
}
