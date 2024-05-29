void cfs_hash_debug_str(struct cfs_hash *hs, struct seq_file *m)
{
	int dist[8] = { 0, };
	int maxdep = -1;
	int maxdepb = -1;
	int total = 0;
	int theta;
	int i;

	cfs_hash_lock(hs, 0);
	theta = __cfs_hash_theta(hs);

	seq_printf(m, "%-*s %5d %5d %5d %d.%03d %d.%03d %d.%03d  0x%02x %6d ",
		   CFS_HASH_BIGNAME_LEN, hs->hs_name,
		   1 << hs->hs_cur_bits, 1 << hs->hs_min_bits,
		   1 << hs->hs_max_bits,
		   __cfs_hash_theta_int(theta), __cfs_hash_theta_frac(theta),
		   __cfs_hash_theta_int(hs->hs_min_theta),
		   __cfs_hash_theta_frac(hs->hs_min_theta),
		   __cfs_hash_theta_int(hs->hs_max_theta),
		   __cfs_hash_theta_frac(hs->hs_max_theta),
		   hs->hs_flags, hs->hs_rehash_count);

	/*
	 * The distribution is a summary of the chained hash depth in
	 * each of the libcfs hash buckets.  Each buckets hsb_count is
	 * divided by the hash theta value and used to generate a
	 * histogram of the hash distribution.  A uniform hash will
	 * result in all hash buckets being close to the average thus
	 * only the first few entries in the histogram will be non-zero.
	 * If you hash function results in a non-uniform hash the will
	 * be observable by outlier bucks in the distribution histogram.
	 *
	 * Uniform hash distribution:		128/128/0/0/0/0/0/0
	 * Non-Uniform hash distribution:	128/125/0/0/0/0/2/1
	 */
	for (i = 0; i < cfs_hash_full_nbkt(hs); i++) {
		struct cfs_hash_bd bd;

		bd.bd_bucket = cfs_hash_full_bkts(hs)[i];
		cfs_hash_bd_lock(hs, &bd, 0);
		if (maxdep < bd.bd_bucket->hsb_depmax) {
			maxdep  = bd.bd_bucket->hsb_depmax;
			maxdepb = ffz(~maxdep);
		}
		total += bd.bd_bucket->hsb_count;
		dist[min(fls(bd.bd_bucket->hsb_count / max(theta, 1)), 7)]++;
		cfs_hash_bd_unlock(hs, &bd, 0);
	}

	seq_printf(m, "%7d %7d %7d ", total, maxdep, maxdepb);
	for (i = 0; i < 8; i++)
		seq_printf(m, "%d%c",  dist[i], (i == 7) ? '\n' : '/');

	cfs_hash_unlock(hs, 0);
}
