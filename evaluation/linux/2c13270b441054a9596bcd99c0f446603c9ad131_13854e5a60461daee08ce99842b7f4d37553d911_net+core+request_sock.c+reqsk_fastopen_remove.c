void reqsk_fastopen_remove(struct sock *sk, struct request_sock *req,
			   bool reset)
{
	struct sock *lsk = tcp_rsk(req)->listener;
	struct fastopen_queue *fastopenq =
	    inet_csk(lsk)->icsk_accept_queue.fastopenq;

	tcp_sk(sk)->fastopen_rsk = NULL;
	spin_lock_bh(&fastopenq->lock);
	fastopenq->qlen--;
	tcp_rsk(req)->listener = NULL;
	if (req->sk)	/* the child socket hasn't been accepted yet */
		goto out;

	if (!reset || lsk->sk_state != TCP_LISTEN) {
		/* If the listener has been closed don't bother with the
		 * special RST handling below.
		 */
		spin_unlock_bh(&fastopenq->lock);
		sock_put(lsk);
		reqsk_put(req);
		return;
	}
	/* Wait for 60secs before removing a req that has triggered RST.
	 * This is a simple defense against TFO spoofing attack - by
	 * counting the req against fastopen.max_qlen, and disabling
	 * TFO when the qlen exceeds max_qlen.
	 *
	 * For more details see CoNext'11 "TCP Fast Open" paper.
	 */
	req->expires = jiffies + 60*HZ;
	if (fastopenq->rskq_rst_head == NULL)
		fastopenq->rskq_rst_head = req;
	else
		fastopenq->rskq_rst_tail->dl_next = req;

	req->dl_next = NULL;
	fastopenq->rskq_rst_tail = req;
	fastopenq->qlen++;
out:
	spin_unlock_bh(&fastopenq->lock);
	sock_put(lsk);
}
