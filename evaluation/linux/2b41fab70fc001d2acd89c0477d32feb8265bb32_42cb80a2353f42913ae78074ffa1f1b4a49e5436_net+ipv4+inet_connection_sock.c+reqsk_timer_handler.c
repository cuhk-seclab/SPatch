static void reqsk_timer_handler(unsigned long data)
{
	struct request_sock *req = (struct request_sock *)data;
	struct sock *sk_listener = req->rsk_listener;
	struct inet_connection_sock *icsk = inet_csk(sk_listener);
	struct request_sock_queue *queue = &icsk->icsk_accept_queue;
	struct listen_sock *lopt = queue->listen_opt;
	int qlen, expire = 0, resend = 0;
	int max_retries, thresh;
	u8 defer_accept;

	if (sk_listener->sk_state != TCP_LISTEN || !lopt) {
		reqsk_put(req);
		return;
	}

	max_retries = icsk->icsk_syn_retries ? : sysctl_tcp_synack_retries;
	thresh = max_retries;
	/* Normally all the openreqs are young and become mature
	 * (i.e. converted to established socket) for first timeout.
	 * If synack was not acknowledged for 1 second, it means
	 * one of the following things: synack was lost, ack was lost,
	 * rtt is high or nobody planned to ack (i.e. synflood).
	 * When server is a bit loaded, queue is populated with old
	 * open requests, reducing effective size of queue.
	 * When server is well loaded, queue size reduces to zero
	 * after several minutes of work. It is not synflood,
	 * it is normal operation. The solution is pruning
	 * too old entries overriding normal timeout, when
	 * situation becomes dangerous.
	 *
	 * Essentially, we reserve half of room for young
	 * embrions; and abort old ones without pity, if old
	 * ones are about to clog our table.
	 */
	qlen = listen_sock_qlen(lopt);
	if (qlen >> (lopt->max_qlen_log - 1)) {
		int young = listen_sock_young(lopt) << 1;

		while (thresh > 2) {
			if (qlen < young)
				break;
			thresh--;
			young <<= 1;
		}
	}
	defer_accept = READ_ONCE(queue->rskq_defer_accept);
	if (defer_accept)
		max_retries = defer_accept;
	syn_ack_recalc(req, thresh, max_retries, defer_accept,
		       &expire, &resend);
	req->rsk_ops->syn_ack_timeout(sk_listener, req);
	if (!expire &&
	    (!resend ||
	     !inet_rtx_syn_ack(sk_listener, req) ||
	     inet_rsk(req)->acked)) {
		unsigned long timeo;

		if (req->num_timeout++ == 0)
			atomic_inc(&lopt->young_dec);
		timeo = min(TCP_TIMEOUT_INIT << req->num_timeout, TCP_RTO_MAX);
		mod_timer_pinned(&req->rsk_timer, jiffies + timeo);
		return;
	}
	inet_csk_reqsk_queue_drop(sk_listener, req);
	reqsk_put(req);
}
