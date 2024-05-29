static int sctp_remaddr_seq_show(struct seq_file *seq, void *v)
{
	struct sctp_association *assoc;
	struct sctp_transport *tsp;

	if (v == SEQ_START_TOKEN) {
		seq_printf(seq, "ADDR ASSOC_ID HB_ACT RTO MAX_PATH_RTX "
				"REM_ADDR_RTX START STATE\n");
		return 0;
	}

	tsp = (struct sctp_transport *)v;
	if (!sctp_transport_hold(tsp))
		return 0;
	assoc = tsp->asoc;

	list_for_each_entry_rcu(tsp, &assoc->peer.transport_addr_list,
				transports) {
		if (tsp->dead)
			continue;
		/*
		 * The remote address (ADDR)
		 */
		tsp->af_specific->seq_dump_addr(seq, &tsp->ipaddr);
		seq_printf(seq, " ");
		/*
		 * The association ID (ASSOC_ID)
		 */
		seq_printf(seq, "%d ", tsp->asoc->assoc_id);

		/*
		 * If the Heartbeat is active (HB_ACT)
		 * Note: 1 = Active, 0 = Inactive
		 */
		seq_printf(seq, "%d ", timer_pending(&tsp->hb_timer));

		/*
		 * Retransmit time out (RTO)
		 */
		seq_printf(seq, "%lu ", tsp->rto);

		/*
		 * Maximum path retransmit count (PATH_MAX_RTX)
		 */
		seq_printf(seq, "%d ", tsp->pathmaxrxt);

		/*
		 * remote address retransmit count (REM_ADDR_RTX)
		 * Note: We don't have a way to tally this at the moment
		 * so lets just leave it as zero for the moment
		 */
		seq_puts(seq, "0 ");

		/*
		 * remote address start time (START).  This is also not
		 * currently implemented, but we can record it with a
		 * jiffies marker in a subsequent patch
		 */
		seq_puts(seq, "0 ");

		/*
		 * The current state of this destination. I.e.
		 * SCTP_ACTIVE, SCTP_INACTIVE, ...
		 */
		seq_printf(seq, "%d", tsp->state);

		seq_printf(seq, "\n");
	}

	sctp_transport_put(tsp);

	return 0;
}
