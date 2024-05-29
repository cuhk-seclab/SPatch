static int rtnl_phys_switch_id_fill(struct sk_buff *skb, struct net_device *dev)
{
	int err;
	struct switchdev_attr attr = {
		.id = SWITCHDEV_ATTR_PORT_PARENT_ID,
		.flags = SWITCHDEV_F_NO_RECURSE,
	};

	err = switchdev_port_attr_get(dev, &attr);
	if (err) {
		if (err == -EOPNOTSUPP)
			return 0;
		return err;
	}

	if (nla_put(skb, IFLA_PHYS_SWITCH_ID, attr.ppid.id_len, attr.ppid.id))
		return -EMSGSIZE;

	return 0;
}
