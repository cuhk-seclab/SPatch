static void can_receive(struct sk_buff *skb, struct net_device *dev)
{
	struct dev_rcv_lists *d;
	int matches;

	/* update statistics */
	can_stats.rx_frames++;
	can_stats.rx_frames_delta++;

	/* create non-zero unique skb identifier together with *skb */
	while (!(can_skb_prv(skb)->skbcnt))
		can_skb_prv(skb)->skbcnt = atomic_inc_return(&skbcounter);

	rcu_read_lock();

	/* deliver the packet to sockets listening on all devices */
	matches = can_rcv_filter(&can_rx_alldev_list, skb);

	/* find receive list for this device */
	d = find_dev_rcv_lists(dev);
	if (d)
		matches += can_rcv_filter(d, skb);

	rcu_read_unlock();

	/* consume the skbuff allocated by the netdevice driver */
	consume_skb(skb);

	if (matches > 0) {
		can_stats.matches++;
		can_stats.matches_delta++;
	}
}
