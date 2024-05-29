unsigned int
nft_do_chain(struct nft_pktinfo *pkt, const struct nf_hook_ops *ops)
{
	const struct nft_chain *chain = ops->priv, *basechain = chain;
	const struct nft_rule *rule;
	const struct nft_expr *expr, *last;
	struct nft_data data[NFT_REG_MAX + 1];
	unsigned int stackptr = 0;
	struct nft_jumpstack jumpstack[NFT_JUMP_STACK_SIZE];
	struct nft_stats *stats;
	int rulenum;
	/*
	 * Cache cursor to avoid problems in case that the cursor is updated
	 * while traversing the ruleset.
	 */
	unsigned int gencursor = ACCESS_ONCE(chain->net->nft.gencursor);

do_chain:
	rulenum = 0;
	rule = list_entry(&chain->rules, struct nft_rule, list);
next_rule:
	data[NFT_REG_VERDICT].verdict = NFT_CONTINUE;
	list_for_each_entry_continue_rcu(rule, &chain->rules, list) {

		/* This rule is not active, skip. */
		if (unlikely(rule->genmask & (1 << gencursor)))
			continue;

		rulenum++;

		nft_rule_for_each_expr(expr, last, rule) {
			if (expr->ops == &nft_cmp_fast_ops)
				nft_cmp_fast_eval(expr, data);
			else if (expr->ops != &nft_payload_fast_ops ||
				 !nft_payload_fast_eval(expr, data, pkt))
				expr->ops->eval(expr, data, pkt);

			if (data[NFT_REG_VERDICT].verdict != NFT_CONTINUE)
				break;
		}

		switch (data[NFT_REG_VERDICT].verdict) {
		case NFT_BREAK:
			data[NFT_REG_VERDICT].verdict = NFT_CONTINUE;
			continue;
		case NFT_CONTINUE:
			nft_trace_packet(pkt, chain, rulenum, NFT_TRACE_RULE);
			continue;
		}
		break;
	}

	switch (data[NFT_REG_VERDICT].verdict & NF_VERDICT_MASK) {
	case NF_ACCEPT:
	case NF_DROP:
	case NF_QUEUE:
		nft_trace_packet(pkt, chain, rulenum, NFT_TRACE_RULE);
		return data[NFT_REG_VERDICT].verdict;
	}

	switch (data[NFT_REG_VERDICT].verdict) {
	case NFT_JUMP:
		BUG_ON(stackptr >= NFT_JUMP_STACK_SIZE);
		jumpstack[stackptr].chain = chain;
		jumpstack[stackptr].rule  = rule;
		jumpstack[stackptr].rulenum = rulenum;
		stackptr++;
		/* fall through */
	case NFT_GOTO:
		nft_trace_packet(pkt, chain, rulenum, NFT_TRACE_RULE);

		chain = data[NFT_REG_VERDICT].chain;
		goto do_chain;
	case NFT_CONTINUE:
		rulenum++;
		/* fall through */
	case NFT_RETURN:
		nft_trace_packet(pkt, chain, rulenum, NFT_TRACE_RETURN);
		break;
	default:
		WARN_ON(1);
	}

	if (stackptr > 0) {
		stackptr--;
		chain = jumpstack[stackptr].chain;
		rule  = jumpstack[stackptr].rule;
		rulenum = jumpstack[stackptr].rulenum;
		goto next_rule;
	}

	nft_trace_packet(pkt, basechain, -1, NFT_TRACE_POLICY);

	rcu_read_lock_bh();
	stats = this_cpu_ptr(rcu_dereference(nft_base_chain(basechain)->stats));
	u64_stats_update_begin(&stats->syncp);
	stats->pkts++;
	stats->bytes += pkt->skb->len;
	u64_stats_update_end(&stats->syncp);
	rcu_read_unlock_bh();

	return nft_base_chain(basechain)->policy;
}
