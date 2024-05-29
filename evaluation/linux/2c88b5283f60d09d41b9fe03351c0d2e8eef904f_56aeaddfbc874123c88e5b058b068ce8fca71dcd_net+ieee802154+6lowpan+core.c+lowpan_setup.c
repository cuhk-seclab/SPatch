static void lowpan_setup(struct net_device *ldev)
{
	ldev->addr_len		= IEEE802154_ADDR_LEN;
	memset(ldev->broadcast, 0xff, IEEE802154_ADDR_LEN);
	ldev->type		= ARPHRD_6LOWPAN;
	/* Frame Control + Sequence Number + Address fields + Security Header */
	ldev->hard_header_len	= 2 + 1 + 20 + 14;
	ldev->needed_tailroom	= 2; /* FCS */
	ldev->mtu		= IPV6_MIN_MTU;
	ldev->priv_flags	|= IFF_NO_QUEUE;
	ldev->flags		= IFF_BROADCAST | IFF_MULTICAST;

	ldev->netdev_ops	= &lowpan_netdev_ops;
	ldev->header_ops	= &lowpan_header_ops;
	ldev->destructor	= free_netdev;
	ldev->features		|= NETIF_F_NETNS_LOCAL;
}
