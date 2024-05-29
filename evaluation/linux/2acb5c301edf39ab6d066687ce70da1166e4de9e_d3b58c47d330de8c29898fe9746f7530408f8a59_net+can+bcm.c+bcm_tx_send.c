static int bcm_tx_send(struct msghdr *msg, int ifindex, struct sock *sk)
{
	struct sk_buff *skb;
	struct net_device *dev;
	int err;

	/* we need a real device to send frames */
	if (!ifindex)
		return -ENODEV;

	skb = alloc_skb(CFSIZ + sizeof(struct can_skb_priv), GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

	can_skb_reserve(skb);

	err = memcpy_from_msg(skb_put(skb, CFSIZ), msg, CFSIZ);
	if (err < 0) {
		kfree_skb(skb);
		return err;
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (!dev) {
		kfree_skb(skb);
		return -ENODEV;
	}

	can_skb_prv(skb)->ifindex = dev->ifindex;
	can_skb_prv(skb)->skbcnt = 0;
	skb->dev = dev;
	can_skb_set_owner(skb, sk);
	err = can_send(skb, 1); /* send with loopback */
	dev_put(dev);

	if (err)
		return err;

	return CFSIZ + MHSIZ;
}
