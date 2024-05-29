static int give_skb_to_upper(struct sk_buff *skb, struct net_device *dev)
{
	struct sk_buff *skb_cp;

	skb_cp = skb_copy(skb, GFP_ATOMIC);
	if (!skb_cp)
		return NET_RX_DROP;

	return netif_rx_ni(skb_cp);
}
