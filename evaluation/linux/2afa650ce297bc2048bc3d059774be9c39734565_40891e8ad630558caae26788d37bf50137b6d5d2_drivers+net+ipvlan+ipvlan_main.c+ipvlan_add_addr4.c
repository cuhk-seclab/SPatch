static int ipvlan_add_addr4(struct ipvl_dev *ipvlan, struct in_addr *ip4_addr)
{
	struct ipvl_addr *addr;

	if (ipvlan_addr_busy(ipvlan, ip4_addr, false)) {
		netif_err(ipvlan, ifup, ipvlan->dev,
			  "Failed to add IPv4=%pI4 on %s intf.\n",
			  ip4_addr, ipvlan->dev->name);
		return -EINVAL;
	}
	addr = kzalloc(sizeof(struct ipvl_addr), GFP_KERNEL);
	if (!addr)
		return -ENOMEM;

	addr->master = ipvlan;
	memcpy(&addr->ip4addr, ip4_addr, sizeof(struct in_addr));
	addr->atype = IPVL_IPV4;
	list_add_tail(&addr->anode, &ipvlan->addrs);
	ipvlan->ipv4cnt++;
	/* If the interface is not up, the address will be added to the hash
	 * list by ipvlan_open.
	 */
	if (netif_running(ipvlan->dev))
		ipvlan_ht_addr_add(ipvlan, addr);
	ipvlan_set_broadcast_mac_filter(ipvlan, true);

	return 0;
}
