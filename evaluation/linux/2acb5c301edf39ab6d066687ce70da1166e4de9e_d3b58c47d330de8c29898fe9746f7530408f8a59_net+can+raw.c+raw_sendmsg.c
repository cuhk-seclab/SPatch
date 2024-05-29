static int raw_sendmsg(struct socket *sock, struct msghdr *msg, size_t size)
{
	struct sock *sk = sock->sk;
	struct raw_sock *ro = raw_sk(sk);
	struct sk_buff *skb;
	struct net_device *dev;
	int ifindex;
	int err;

	if (msg->msg_name) {
		DECLARE_SOCKADDR(struct sockaddr_can *, addr, msg->msg_name);

		if (msg->msg_namelen < sizeof(*addr))
			return -EINVAL;

		if (addr->can_family != AF_CAN)
			return -EINVAL;

		ifindex = addr->can_ifindex;
	} else
		ifindex = ro->ifindex;

	if (ro->fd_frames) {
		if (unlikely(size != CANFD_MTU && size != CAN_MTU))
			return -EINVAL;
	} else {
		if (unlikely(size != CAN_MTU))
			return -EINVAL;
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (!dev)
		return -ENXIO;

	skb = sock_alloc_send_skb(sk, size + sizeof(struct can_skb_priv),
				  msg->msg_flags & MSG_DONTWAIT, &err);
	if (!skb)
		goto put_dev;

	can_skb_reserve(skb);
	can_skb_prv(skb)->ifindex = dev->ifindex;
	can_skb_prv(skb)->skbcnt = 0;

	err = memcpy_from_msg(skb_put(skb, size), msg, size);
	if (err < 0)
		goto free_skb;

	sock_tx_timestamp(sk, &skb_shinfo(skb)->tx_flags);

	skb->dev = dev;
	skb->sk  = sk;
	skb->priority = sk->sk_priority;

	err = can_send(skb, ro->loopback);

	dev_put(dev);

	if (err)
		goto send_failed;

	return size;

free_skb:
	kfree_skb(skb);
put_dev:
	dev_put(dev);
send_failed:
	return err;
}
