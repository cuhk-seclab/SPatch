void tipc_link_retransmit(struct tipc_link *l_ptr, struct sk_buff *skb,
			  u32 retransmits)
{
	struct tipc_msg *msg;

	if (!skb)
		return;

	msg = buf_msg(skb);

	/* Detect repeated retransmit failures */
	if (l_ptr->last_retransmitted == msg_seqno(msg)) {
		if (++l_ptr->stale_count > 100) {
			link_retransmit_failure(l_ptr, skb);
			return;
		}
	} else {
		l_ptr->last_retransmitted = msg_seqno(msg);
		l_ptr->stale_count = 1;
	}

	skb_queue_walk_from(&l_ptr->outqueue, skb) {
		if (!retransmits || skb == l_ptr->next_out)
			break;
		msg = buf_msg(skb);
		msg_set_ack(msg, mod(l_ptr->next_in_no - 1));
		msg_set_bcast_ack(msg, l_ptr->owner->bclink.last_in);
		tipc_bearer_send(l_ptr->owner->net, l_ptr->bearer_id, skb,
				 &l_ptr->media_addr);
		retransmits--;
		l_ptr->stats.retransmitted++;
	}
}
