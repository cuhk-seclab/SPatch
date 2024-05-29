static int
sctp_conn_schedule(int af, struct sk_buff *skb, struct ip_vs_proto_data *pd,
		   int *verdict, struct ip_vs_conn **cpp,
		   struct ip_vs_iphdr *iph)
{
	struct net *net;
	struct ip_vs_service *svc;
	struct netns_ipvs *ipvs;
	sctp_chunkhdr_t _schunkh, *sch;
	sctp_sctphdr_t *sh, _sctph;

		sh = skb_header_pointer(skb, iph->len, sizeof(_sctph), &_sctph);
		if (sh) {
			sch = skb_header_pointer(
				skb, iph->len + sizeof(sctp_sctphdr_t),
				sizeof(_schunkh), &_schunkh);
			if (sch && (sch->type == SCTP_CID_INIT ||
				    sysctl_sloppy_sctp(ipvs)))
				ports = &sh->source;
		}
	} else {
		ports = skb_header_pointer(
	if (ip_vs_iph_icmp(iph)) {
		return 0;
	}

	sch = skb_header_pointer(skb, iph->len + sizeof(sctp_sctphdr_t),
	if (sch == NULL) {
		*verdict = NF_DROP;
		return 0;
	}

	net = skb_net(skb);
	ipvs = net_ipvs(net);
	rcu_read_lock();
	if (likely(!ip_vs_iph_inverse(iph)))
		svc = ip_vs_service_find(net, af, skb->mark, iph->protocol,
					 &iph->daddr, ports[1]);
	else
		svc = ip_vs_service_find(net, af, skb->mark, iph->protocol,
					 &iph->saddr, ports[0]);
	if (svc) {
		int ignored;

		if (ip_vs_todrop(ipvs)) {
			/*
			 * It seems that we are very loaded.
			 * We have to drop this packet :(
			 */
			rcu_read_unlock();
			*verdict = NF_DROP;
			return 0;
		}
		/*
		 * Let the virtual server select a real server for the
		 * incoming connection, and create a connection entry.
		 */
		*cpp = ip_vs_schedule(svc, skb, pd, &ignored, iph);
		if (!*cpp && ignored <= 0) {
			if (!ignored)
				*verdict = ip_vs_leave(svc, skb, pd, iph);
			else
				*verdict = NF_DROP;
			rcu_read_unlock();
			return 0;
		}
	}
	rcu_read_unlock();
	/* NF_ACCEPT */
	return 1;
}
