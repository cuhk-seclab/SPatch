static int pppoe_rcv_core(struct sock *sk, struct sk_buff *skb)
{
	struct pppox_sock *po = pppox_sk(sk);
	struct pppox_sock *relay_po;

	/* Backlog receive. Semantics of backlog rcv preclude any code from
	 * executing in lock_sock()/release_sock() bounds; meaning sk->sk_state
	 * can't change.
	 */

	if (skb->pkt_type == PACKET_OTHERHOST)
		goto abort_kfree;

	if (sk->sk_state & PPPOX_BOUND) {
		ppp_input(&po->chan, skb);
	} else if (sk->sk_state & PPPOX_RELAY) {
		relay_po = get_item_by_addr(sock_net(sk),
					    &po->pppoe_relay);
		if (relay_po == NULL)
			goto abort_kfree;

		if ((sk_pppox(relay_po)->sk_state & PPPOX_CONNECTED) == 0)
			goto abort_put;

		if (!__pppoe_xmit(sk_pppox(relay_po), skb))
			goto abort_put;
	} else {
		if (sock_queue_rcv_skb(sk, skb))
			goto abort_kfree;
	}

	return NET_RX_SUCCESS;

abort_put:
	sock_put(sk_pppox(relay_po));

abort_kfree:
	kfree_skb(skb);
	return NET_RX_DROP;
}
