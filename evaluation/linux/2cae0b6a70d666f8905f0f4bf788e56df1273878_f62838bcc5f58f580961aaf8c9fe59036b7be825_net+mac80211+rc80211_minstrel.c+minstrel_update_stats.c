static void
minstrel_update_stats(struct minstrel_priv *mp, struct minstrel_sta_info *mi)
{
	u8 tmp_tp_rate[MAX_THR_RATES];
	u8 tmp_prob_rate = 0;
	u32 usecs;
	int i;

	for (i = 0; i < MAX_THR_RATES; i++)
	    tmp_tp_rate[i] = 0;

	for (i = 0; i < mi->n_rates; i++) {
		struct minstrel_rate *mr = &mi->r[i];
		struct minstrel_rate_stats *mrs = &mi->r[i].stats;

		usecs = mr->perfect_tx_time;
		if (!usecs)
			usecs = 1000000;

		if (unlikely(mrs->attempts > 0)) {
			mrs->sample_skipped = 0;
			mrs->cur_prob = MINSTREL_FRAC(mrs->success,
						      mrs->attempts);
			mrs->succ_hist += mrs->success;
			mrs->att_hist += mrs->attempts;
			mrs->probability = minstrel_ewma(mrs->probability,
							 mrs->cur_prob,
							 EWMA_LEVEL);
		} else
			mrs->sample_skipped++;

		mrs->last_success = mrs->success;
		mrs->last_attempts = mrs->attempts;
		mrs->success = 0;
		mrs->attempts = 0;

		/* Update throughput per rate, reset thr. below 10% success */
		if (mrs->probability < MINSTREL_FRAC(10, 100))
			mrs->cur_tp = 0;
		else
			mrs->cur_tp = mrs->probability * (1000000 / usecs);

		/* Sample less often below the 10% chance of success.
		 * Sample less often above the 95% chance of success. */
		if (mrs->probability > MINSTREL_FRAC(95, 100) ||
		    mrs->probability < MINSTREL_FRAC(10, 100)) {
			mr->adjusted_retry_count = mrs->retry_count >> 1;
			if (mr->adjusted_retry_count > 2)
				mr->adjusted_retry_count = 2;
			mr->sample_limit = 4;
		} else {
			mr->sample_limit = -1;
			mr->adjusted_retry_count = mrs->retry_count;
		}
		if (!mr->adjusted_retry_count)
			mr->adjusted_retry_count = 2;

		minstrel_sort_best_tp_rates(mi, i, tmp_tp_rate);

		/* To determine the most robust rate (max_prob_rate) used at
		 * 3rd mmr stage we distinct between two cases:
		 * (1) if any success probabilitiy >= 95%, out of those rates
		 * choose the maximum throughput rate as max_prob_rate
		 * (2) if all success probabilities < 95%, the rate with
		 * highest success probability is chosen as max_prob_rate */
		if (mrs->probability >= MINSTREL_FRAC(95, 100)) {
			if (mrs->cur_tp >= mi->r[tmp_prob_rate].stats.cur_tp)
				tmp_prob_rate = i;
		} else {
			if (mrs->probability >= mi->r[tmp_prob_rate].stats.probability)
				tmp_prob_rate = i;
		}
	}

	/* Assign the new rate set */
	memcpy(mi->max_tp_rate, tmp_tp_rate, sizeof(mi->max_tp_rate));
	mi->max_prob_rate = tmp_prob_rate;

#ifdef CONFIG_MAC80211_DEBUGFS
	/* use fixed index if set */
	if (mp->fixed_rate_idx != -1) {
		mi->max_tp_rate[0] = mp->fixed_rate_idx;
		mi->max_tp_rate[1] = mp->fixed_rate_idx;
		mi->max_prob_rate = mp->fixed_rate_idx;
	}
#endif

	/* Reset update timer */
	mi->stats_update = jiffies;

	minstrel_update_rates(mp, mi);
}
