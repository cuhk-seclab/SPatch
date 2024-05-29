static bool link_schedule_user(struct tipc_link *link, u32 oport,
			       uint chain_sz, uint imp)
{
	struct sk_buff *buf;

	buf = tipc_msg_create(SOCK_WAKEUP, 0, INT_H_SIZE, 0,
			      link_own_addr(link), link_own_addr(link),
			      oport, 0, 0);
	if (!buf)
		return false;
	TIPC_SKB_CB(buf)->chain_sz = chain_sz;
	TIPC_SKB_CB(buf)->chain_imp = imp;
	skb_queue_tail(&link->wakeupq, buf);
	link->stats.link_congs++;
	return true;
}
