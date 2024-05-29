static int bond_fill_info(struct sk_buff *skb,
			  const struct net_device *bond_dev)
{
	struct bonding *bond = netdev_priv(bond_dev);
	unsigned int packets_per_slave;
	int ifindex, i, targets_added;
	struct nlattr *targets;
	struct slave *primary;

	if (nla_put_u8(skb, IFLA_BOND_MODE, BOND_MODE(bond)))
		goto nla_put_failure;

	ifindex = bond_option_active_slave_get_ifindex(bond);
	if (ifindex && nla_put_u32(skb, IFLA_BOND_ACTIVE_SLAVE, ifindex))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_MIIMON, bond->params.miimon))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_UPDELAY,
			bond->params.updelay * bond->params.miimon))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_DOWNDELAY,
			bond->params.downdelay * bond->params.miimon))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_USE_CARRIER, bond->params.use_carrier))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_ARP_INTERVAL, bond->params.arp_interval))
		goto nla_put_failure;

	targets = nla_nest_start(skb, IFLA_BOND_ARP_IP_TARGET);
	if (!targets)
		goto nla_put_failure;

	targets_added = 0;
	for (i = 0; i < BOND_MAX_ARP_TARGETS; i++) {
		if (bond->params.arp_targets[i]) {
			nla_put_be32(skb, i, bond->params.arp_targets[i]);
			targets_added = 1;
		}
	}

	if (targets_added)
		nla_nest_end(skb, targets);
	else
		nla_nest_cancel(skb, targets);

	if (nla_put_u32(skb, IFLA_BOND_ARP_VALIDATE, bond->params.arp_validate))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_ARP_ALL_TARGETS,
			bond->params.arp_all_targets))
		goto nla_put_failure;

	primary = rtnl_dereference(bond->primary_slave);
	if (primary &&
	    nla_put_u32(skb, IFLA_BOND_PRIMARY, primary->dev->ifindex))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_PRIMARY_RESELECT,
		       bond->params.primary_reselect))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_FAIL_OVER_MAC,
		       bond->params.fail_over_mac))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_XMIT_HASH_POLICY,
		       bond->params.xmit_policy))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_RESEND_IGMP,
		        bond->params.resend_igmp))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_NUM_PEER_NOTIF,
		       bond->params.num_peer_notif))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_ALL_SLAVES_ACTIVE,
		       bond->params.all_slaves_active))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_MIN_LINKS,
			bond->params.min_links))
		goto nla_put_failure;

	if (nla_put_u32(skb, IFLA_BOND_LP_INTERVAL,
			bond->params.lp_interval))
		goto nla_put_failure;

	packets_per_slave = bond->params.packets_per_slave;
	if (nla_put_u32(skb, IFLA_BOND_PACKETS_PER_SLAVE,
			packets_per_slave))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_AD_LACP_RATE,
		       bond->params.lacp_fast))
		goto nla_put_failure;

	if (nla_put_u8(skb, IFLA_BOND_AD_SELECT,
		       bond->params.ad_select))
		goto nla_put_failure;

	if (BOND_MODE(bond) == BOND_MODE_8023AD) {
		struct ad_info info;

		if (capable(CAP_NET_ADMIN)) {
			if (nla_put_u16(skb, IFLA_BOND_AD_ACTOR_SYS_PRIO,
					bond->params.ad_actor_sys_prio))
				goto nla_put_failure;

			if (nla_put_u16(skb, IFLA_BOND_AD_USER_PORT_KEY,
					bond->params.ad_user_port_key))
				goto nla_put_failure;

			if (nla_put(skb, IFLA_BOND_AD_ACTOR_SYSTEM,
				    sizeof(bond->params.ad_actor_system),
				    &bond->params.ad_actor_system))
				goto nla_put_failure;
		}
		if (!bond_3ad_get_active_agg_info(bond, &info)) {
			struct nlattr *nest;

			nest = nla_nest_start(skb, IFLA_BOND_AD_INFO);
			if (!nest)
				goto nla_put_failure;

			if (nla_put_u16(skb, IFLA_BOND_AD_INFO_AGGREGATOR,
					info.aggregator_id))
				goto nla_put_failure;
			if (nla_put_u16(skb, IFLA_BOND_AD_INFO_NUM_PORTS,
					info.ports))
				goto nla_put_failure;
			if (nla_put_u16(skb, IFLA_BOND_AD_INFO_ACTOR_KEY,
					info.actor_key))
				goto nla_put_failure;
			if (nla_put_u16(skb, IFLA_BOND_AD_INFO_PARTNER_KEY,
					info.partner_key))
				goto nla_put_failure;
			if (nla_put(skb, IFLA_BOND_AD_INFO_PARTNER_MAC,
				    sizeof(info.partner_system),
				    &info.partner_system))
				goto nla_put_failure;

			nla_nest_end(skb, nest);
		}
	}

	return 0;

nla_put_failure:
	return -EMSGSIZE;
}
