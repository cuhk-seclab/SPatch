bool tipc_msg_bundle(struct sk_buff_head *list, struct sk_buff *skb, u32 mtu)
{
	struct sk_buff *bskb = skb_peek_tail(list);
	struct tipc_msg *bmsg = buf_msg(bskb);
	struct tipc_msg *msg = buf_msg(skb);
	unsigned int bsz = msg_size(bmsg);
	unsigned int msz = msg_size(msg);
	u32 start = align(bsz);
	u32 max = mtu - INT_H_SIZE;
	u32 pad = start - bsz;

	if (likely(msg_user(msg) == MSG_FRAGMENTER))
		return false;
	if (unlikely(msg_user(msg) == CHANGEOVER_PROTOCOL))
		return false;
	if (unlikely(msg_user(msg) == BCAST_PROTOCOL))
		return false;
	if (likely(msg_user(bmsg) != MSG_BUNDLER))
		return false;
	if (likely(!TIPC_SKB_CB(bskb)->bundling))
		return false;
	if (unlikely(skb_tailroom(bskb) < (pad + msz)))
		return false;
	if (unlikely(max < (start + msz)))
		return false;

	skb_put(bskb, pad + msz);
	skb_copy_to_linear_data_offset(bskb, start, skb->data, msz);
	msg_set_size(bmsg, start + msz);
	msg_set_msgcnt(bmsg, msg_msgcnt(bmsg) + 1);
	kfree_skb(skb);
	return true;
}
