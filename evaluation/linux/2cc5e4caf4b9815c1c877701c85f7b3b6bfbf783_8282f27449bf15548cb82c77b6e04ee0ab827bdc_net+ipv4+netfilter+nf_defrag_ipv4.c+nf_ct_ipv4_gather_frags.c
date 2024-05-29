static int nf_ct_ipv4_gather_frags(struct net *net, struct sk_buff *skb,
				   u_int32_t user)
{
	int err;

	local_bh_disable();
	err = ip_defrag(net, skb, user);
	local_bh_enable();

	if (!err) {
		ip_send_check(ip_hdr(skb));
		skb->ignore_df = 1;
	}

	return err;
}
