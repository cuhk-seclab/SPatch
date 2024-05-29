static struct rt6_info *find_rr_leaf(struct fib6_node *fn,
				     struct rt6_info *rr_head,
				     u32 metric, int oif, int strict,
				     bool *do_rr)
{
	struct rt6_info *rt, *match;
	int mpri = -1;

	match = NULL;
	for (rt = rr_head; rt && rt->rt6i_metric == metric;
	     rt = rt->dst.rt6_next)
		match = find_match(rt, oif, strict, &mpri, match, do_rr);
	for (rt = fn->leaf; rt && rt != rr_head && rt->rt6i_metric == metric;
	     rt = rt->dst.rt6_next)
		match = find_match(rt, oif, strict, &mpri, match, do_rr);

	return match;
}
