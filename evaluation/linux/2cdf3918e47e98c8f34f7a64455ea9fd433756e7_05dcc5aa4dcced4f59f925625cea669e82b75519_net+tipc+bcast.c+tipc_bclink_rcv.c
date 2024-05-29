void tipc_bclink_rcv(struct net *net, struct sk_buff *buf)
{
	struct tipc_net *tn = net_generic(net, tipc_net_id);
	struct tipc_link *bcl = tn->bcl;
	struct tipc_msg *msg = buf_msg(buf);
	struct tipc_node *node;
	u32 next_in;
	u32 seqno;
	int deferred = 0;
	int pos = 0;
	struct sk_buff *iskb;
	struct sk_buff_head *arrvq, *inputq;

	/* Screen out unwanted broadcast messages */
	if (msg_mc_netid(msg) != tn->net_id)
		goto exit;

	node = tipc_node_find(net, msg_prevnode(msg));
	if (unlikely(!node))
		goto exit;

	tipc_node_lock(node);
	if (unlikely(!node->bclink.recv_permitted))
		goto unlock;

	/* Handle broadcast protocol message */
	if (unlikely(msg_user(msg) == BCAST_PROTOCOL)) {
		if (msg_type(msg) != STATE_MSG)
			goto unlock;
		if (msg_destnode(msg) == tn->own_addr) {
			tipc_bclink_acknowledge(node, msg_bcast_ack(msg));
			tipc_node_unlock(node);
			tipc_bclink_lock(net);
			bcl->stats.recv_nacks++;
			tn->bclink->retransmit_to = node;
			bclink_retransmit_pkt(tn, msg_bcgap_after(msg),
					      msg_bcgap_to(msg));
			tipc_bclink_unlock(net);
		} else {
			tipc_node_unlock(node);
			bclink_peek_nack(net, msg);
		}
		goto exit;
	}

	/* Handle in-sequence broadcast message */
	seqno = msg_seqno(msg);
	next_in = mod(node->bclink.last_in + 1);
	arrvq = &tn->bclink->arrvq;
	inputq = &tn->bclink->inputq;

	if (likely(seqno == next_in)) {
receive:
		/* Deliver message to destination */
		if (likely(msg_isdata(msg))) {
			tipc_bclink_lock(net);
			bclink_accept_pkt(node, seqno);
			spin_lock_bh(&inputq->lock);
			__skb_queue_tail(arrvq, buf);
			spin_unlock_bh(&inputq->lock);
			node->action_flags |= TIPC_BCAST_MSG_EVT;
			tipc_bclink_unlock(net);
			tipc_node_unlock(node);
		} else if (msg_user(msg) == MSG_BUNDLER) {
			tipc_bclink_lock(net);
			bclink_accept_pkt(node, seqno);
			bcl->stats.recv_bundles++;
			bcl->stats.recv_bundled += msg_msgcnt(msg);
			pos = 0;
			while (tipc_msg_extract(buf, &iskb, &pos)) {
				spin_lock_bh(&inputq->lock);
				__skb_queue_tail(arrvq, iskb);
				spin_unlock_bh(&inputq->lock);
			}
			node->action_flags |= TIPC_BCAST_MSG_EVT;
			tipc_bclink_unlock(net);
			tipc_node_unlock(node);
		} else if (msg_user(msg) == MSG_FRAGMENTER) {
			tipc_buf_append(&node->bclink.reasm_buf, &buf);
			if (unlikely(!buf && !node->bclink.reasm_buf))
				goto unlock;
			tipc_bclink_lock(net);
			bclink_accept_pkt(node, seqno);
			bcl->stats.recv_fragments++;
			if (buf) {
				bcl->stats.recv_fragmented++;
				msg = buf_msg(buf);
				tipc_bclink_unlock(net);
				goto receive;
			}
			tipc_bclink_unlock(net);
			tipc_node_unlock(node);
		} else {
			tipc_bclink_lock(net);
			bclink_accept_pkt(node, seqno);
			tipc_bclink_unlock(net);
			tipc_node_unlock(node);
			kfree_skb(buf);
		}
		buf = NULL;

		/* Determine new synchronization state */
		tipc_node_lock(node);
		if (unlikely(!tipc_node_is_up(node)))
			goto unlock;

		if (node->bclink.last_in == node->bclink.last_sent)
			goto unlock;

		if (skb_queue_empty(&node->bclink.deferred_queue)) {
			node->bclink.oos_state = 1;
			goto unlock;
		}

		msg = buf_msg(skb_peek(&node->bclink.deferred_queue));
		seqno = msg_seqno(msg);
		next_in = mod(next_in + 1);
		if (seqno != next_in)
			goto unlock;

		/* Take in-sequence message from deferred queue & deliver it */
		buf = __skb_dequeue(&node->bclink.deferred_queue);
		goto receive;
	}

	/* Handle out-of-sequence broadcast message */
	if (less(next_in, seqno)) {
		deferred = tipc_link_defer_pkt(&node->bclink.deferred_queue,
					       buf);
		bclink_update_last_sent(node, seqno);
		buf = NULL;
	}

	tipc_bclink_lock(net);

	if (deferred)
		bcl->stats.deferred_recv++;
	else
		bcl->stats.duplicates++;

	tipc_bclink_unlock(net);

unlock:
	tipc_node_unlock(node);
exit:
	kfree_skb(buf);
}
