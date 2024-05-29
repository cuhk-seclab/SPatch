int ipv6_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
{
	const struct ipv6hdr *hdr;
	u32 pkt_len;
	struct inet6_dev *idev;
	struct net *net = dev_net(skb->dev);

	if (skb->pkt_type == PACKET_OTHERHOST) {
		kfree_skb(skb);
		return NET_RX_DROP;
	}

	rcu_read_lock();

	idev = __in6_dev_get(skb->dev);

	IP6_UPD_PO_STATS_BH(net, idev, IPSTATS_MIB_IN, skb->len);

	if ((skb = skb_share_check(skb, GFP_ATOMIC)) == NULL ||
	    !idev || unlikely(idev->cnf.disable_ipv6)) {
		IP6_INC_STATS_BH(net, idev, IPSTATS_MIB_INDISCARDS);
		goto drop;
	}

	memset(IP6CB(skb), 0, sizeof(struct inet6_skb_parm));

	/*
	 * Store incoming device index. When the packet will
	 * be queued, we cannot refer to skb->dev anymore.
	 *
	 * BTW, when we send a packet for our own local address on a
	 * non-loopback interface (e.g. ethX), it is being delivered
	 * via the loopback interface (lo) here; skb->dev = loopback_dev.
	 * It, however, should be considered as if it is being
	 * arrived via the sending interface (ethX), because of the
	 * nature of scoping architecture. --yoshfuji
	 */
	IP6CB(skb)->iif = skb_valid_dst(skb) ? ip6_dst_idev(skb_dst(skb))->dev->ifindex : dev->ifindex;

	if (unlikely(!pskb_may_pull(skb, sizeof(*hdr))))
		goto err;

	hdr = ipv6_hdr(skb);

	if (hdr->version != 6)
		goto err;

	IP6_ADD_STATS_BH(net, idev,
			 IPSTATS_MIB_NOECTPKTS +
				(ipv6_get_dsfield(hdr) & INET_ECN_MASK),
			 max_t(unsigned short, 1, skb_shinfo(skb)->gso_segs));
	/*
	 * RFC4291 2.5.3
	 * A packet received on an interface with a destination address
	 * of loopback must be dropped.
	 */
	if (!(dev->flags & IFF_LOOPBACK) &&
	    ipv6_addr_loopback(&hdr->daddr))
		goto err;

	/* RFC4291 Errata ID: 3480
	 * Interface-Local scope spans only a single interface on a
	 * node and is useful only for loopback transmission of
	 * multicast.  Packets with interface-local scope received
	 * from another node must be discarded.
	 */
	if (!(skb->pkt_type == PACKET_LOOPBACK ||
	      dev->flags & IFF_LOOPBACK) &&
	    ipv6_addr_is_multicast(&hdr->daddr) &&
	    IPV6_ADDR_MC_SCOPE(&hdr->daddr) == 1)
		goto err;

	/* RFC4291 2.7
	 * Nodes must not originate a packet to a multicast address whose scope
	 * field contains the reserved value 0; if such a packet is received, it
	 * must be silently dropped.
	 */
	if (ipv6_addr_is_multicast(&hdr->daddr) &&
	    IPV6_ADDR_MC_SCOPE(&hdr->daddr) == 0)
		goto err;

	/*
	 * RFC4291 2.7
	 * Multicast addresses must not be used as source addresses in IPv6
	 * packets or appear in any Routing header.
	 */
	if (ipv6_addr_is_multicast(&hdr->saddr))
		goto err;

	skb->transport_header = skb->network_header + sizeof(*hdr);
	IP6CB(skb)->nhoff = offsetof(struct ipv6hdr, nexthdr);

	pkt_len = ntohs(hdr->payload_len);

	/* pkt_len may be zero if Jumbo payload option is present */
	if (pkt_len || hdr->nexthdr != NEXTHDR_HOP) {
		if (pkt_len + sizeof(struct ipv6hdr) > skb->len) {
			IP6_INC_STATS_BH(net,
					 idev, IPSTATS_MIB_INTRUNCATEDPKTS);
			goto drop;
		}
		if (pskb_trim_rcsum(skb, pkt_len + sizeof(struct ipv6hdr))) {
			IP6_INC_STATS_BH(net, idev, IPSTATS_MIB_INHDRERRORS);
			goto drop;
		}
		hdr = ipv6_hdr(skb);
	}

	if (hdr->nexthdr == NEXTHDR_HOP) {
		if (ipv6_parse_hopopts(skb) < 0) {
			IP6_INC_STATS_BH(net, idev, IPSTATS_MIB_INHDRERRORS);
			rcu_read_unlock();
			return NET_RX_DROP;
		}
	}

	rcu_read_unlock();

	/* Must drop socket now because of tproxy. */
	skb_orphan(skb);

	return NF_HOOK(NFPROTO_IPV6, NF_INET_PRE_ROUTING, NULL, skb,
		       dev, NULL,
		       ip6_rcv_finish);
err:
	IP6_INC_STATS_BH(net, idev, IPSTATS_MIB_INHDRERRORS);
drop:
	rcu_read_unlock();
	kfree_skb(skb);
	return NET_RX_DROP;
}
