unsigned int can_get_echo_skb(struct net_device *dev, unsigned int idx)
{
	struct can_priv *priv = netdev_priv(dev);

	BUG_ON(idx >= priv->echo_skb_max);

	if (priv->echo_skb[idx]) {
		struct sk_buff *skb = priv->echo_skb[idx];
		struct can_frame *cf = (struct can_frame *)skb->data;
		u8 dlc = cf->can_dlc;

		netif_rx(priv->echo_skb[idx]);
		priv->echo_skb[idx] = NULL;

		return dlc;
	}

	return 0;
}
