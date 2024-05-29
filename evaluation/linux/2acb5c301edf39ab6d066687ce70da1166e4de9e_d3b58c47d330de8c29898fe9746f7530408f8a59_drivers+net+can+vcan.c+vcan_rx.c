static void vcan_rx(struct sk_buff *skb, struct net_device *dev)
{
	struct canfd_frame *cfd = (struct canfd_frame *)skb->data;
	struct net_device_stats *stats = &dev->stats;

	stats->rx_packets++;
	stats->rx_bytes += cfd->len;

	skb->pkt_type  = PACKET_BROADCAST;
	skb->dev       = dev;
	skb->ip_summed = CHECKSUM_UNNECESSARY;

	netif_rx_ni(skb);
}
