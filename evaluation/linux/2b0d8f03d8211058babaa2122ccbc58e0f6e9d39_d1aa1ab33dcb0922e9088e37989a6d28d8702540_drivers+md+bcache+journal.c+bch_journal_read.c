int bch_journal_read(struct cache_set *c, struct list_head *list)
{
#define read_bucket(b)							\
	({								\
		int ret = journal_read_bucket(ca, list, b);		\
		__set_bit(b, bitmap);					\
		if (ret < 0)						\
			return ret;					\
		ret;							\
	})

	struct cache *ca;
	unsigned iter;

	for_each_cache(ca, c, iter) {
		struct journal_device *ja = &ca->journal;
		DECLARE_BITMAP(bitmap, SB_JOURNAL_BUCKETS);
		unsigned i, l, r, m;
		uint64_t seq;

		bitmap_zero(bitmap, SB_JOURNAL_BUCKETS);
		pr_debug("%u journal buckets", ca->sb.njournal_buckets);

		/*
		 * Read journal buckets ordered by golden ratio hash to quickly
		 * find a sequence of buckets with valid journal entries
		 */
		for (i = 0; i < ca->sb.njournal_buckets; i++) {
			l = (i * 2654435769U) % ca->sb.njournal_buckets;

			if (test_bit(l, bitmap))
				break;

			if (read_bucket(l))
				goto bsearch;
		}

		/*
		 * If that fails, check all the buckets we haven't checked
		 * already
		 */
		pr_debug("falling back to linear search");

		for (l = find_first_zero_bit(bitmap, ca->sb.njournal_buckets);
		     l < ca->sb.njournal_buckets;
		     l = find_next_zero_bit(bitmap, ca->sb.njournal_buckets, l + 1))
			if (read_bucket(l))
				goto bsearch;

		/* no journal entries on this device? */
		if (l == ca->sb.njournal_buckets)
			continue;
bsearch:
		BUG_ON(list_empty(list));

		/* Binary search */
		m = l;
		r = find_next_bit(bitmap, ca->sb.njournal_buckets, l + 1);
		pr_debug("starting binary search, l %u r %u", l, r);

		while (l + 1 < r) {
			seq = list_entry(list->prev, struct journal_replay,
					 list)->j.seq;

			m = (l + r) >> 1;
			read_bucket(m);

			if (seq != list_entry(list->prev, struct journal_replay,
					      list)->j.seq)
				l = m;
			else
				r = m;
		}

		/*
		 * Read buckets in reverse order until we stop finding more
		 * journal entries
		 */
		pr_debug("finishing up: m %u njournal_buckets %u",
			 m, ca->sb.njournal_buckets);
		l = m;

		while (1) {
			if (!l--)
				l = ca->sb.njournal_buckets - 1;

			if (l == m)
				break;

			if (test_bit(l, bitmap))
				continue;

			if (!read_bucket(l))
				break;
		}

		seq = 0;

		for (i = 0; i < ca->sb.njournal_buckets; i++)
			if (ja->seq[i] > seq) {
				seq = ja->seq[i];
				/*
				 * When journal_reclaim() goes to allocate for
				 * the first time, it'll use the bucket after
				 * ja->cur_idx
				 */
				ja->cur_idx = i;
				ja->last_idx = ja->discard_idx = (i + 1) %
					ca->sb.njournal_buckets;

			}
	}

	if (!list_empty(list))
		c->journal.seq = list_entry(list->prev,
					    struct journal_replay,
					    list)->j.seq;

	return 0;
#undef read_bucket
}
