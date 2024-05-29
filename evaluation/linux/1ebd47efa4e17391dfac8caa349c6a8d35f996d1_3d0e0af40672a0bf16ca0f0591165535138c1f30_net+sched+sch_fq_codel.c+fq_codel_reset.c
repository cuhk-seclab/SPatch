static void fq_codel_reset(struct Qdisc *sch)
{
	struct sk_buff *skb;

	INIT_LIST_HEAD(&q->old_flows);
	for (i = 0; i < q->flows_cnt; i++) {
		struct fq_codel_flow *flow = q->flows + i;

		while (flow->head) {
			struct sk_buff *skb = dequeue_head(flow);

			qdisc_qstats_backlog_dec(sch, skb);
			kfree_skb(skb);
		}

		INIT_LIST_HEAD(&flow->flowchain);
		codel_vars_init(&flow->cvars);
	}
	memset(q->backlogs, 0, q->flows_cnt * sizeof(u32));
	while ((skb = fq_codel_dequeue(sch)) != NULL)
		kfree_skb(skb);
}
