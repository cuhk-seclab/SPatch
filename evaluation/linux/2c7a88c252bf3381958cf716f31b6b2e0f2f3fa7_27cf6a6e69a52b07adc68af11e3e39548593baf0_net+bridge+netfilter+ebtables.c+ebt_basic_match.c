static inline int
ebt_basic_match(const struct ebt_entry *e, const struct sk_buff *skb,
                const struct net_device *in, const struct net_device *out)
{
	const struct ethhdr *h = eth_hdr(skb);
	const struct net_bridge_port *p;
	__be16 ethproto;
	int verdict, i;

	if (skb_vlan_tag_present(skb))
		ethproto = htons(ETH_P_8021Q);
	else
		ethproto = h->h_proto;

	if (e->bitmask & EBT_802_3) {
		if (FWINV2(eth_proto_is_802_3(ethproto), EBT_IPROTO))
			return 1;
	} else if (!(e->bitmask & EBT_NOPROTO) &&
	   FWINV2(e->ethproto != ethproto, EBT_IPROTO))
		return 1;

	if (FWINV2(ebt_dev_check(e->in, in), EBT_IIN))
		return 1;
	if (FWINV2(ebt_dev_check(e->out, out), EBT_IOUT))
		return 1;
	/* rcu_read_lock()ed by nf_hook_slow */
	if (in && (p = br_port_get_rcu(in)) != NULL &&
	    FWINV2(ebt_dev_check(e->logical_in, p->br->dev), EBT_ILOGICALIN))
		return 1;
	if (out && (p = br_port_get_rcu(out)) != NULL &&
	    FWINV2(ebt_dev_check(e->logical_out, p->br->dev), EBT_ILOGICALOUT))
		return 1;

	if (e->bitmask & EBT_SOURCEMAC) {
		verdict = 0;
		for (i = 0; i < 6; i++)
			verdict |= (h->h_source[i] ^ e->sourcemac[i]) &
			   e->sourcemsk[i];
		if (FWINV2(verdict != 0, EBT_ISOURCE) )
			return 1;
	}
	if (e->bitmask & EBT_DESTMAC) {
		verdict = 0;
		for (i = 0; i < 6; i++)
			verdict |= (h->h_dest[i] ^ e->destmac[i]) &
			   e->destmsk[i];
		if (FWINV2(verdict != 0, EBT_IDEST) )
			return 1;
	}
	return 0;
}
