static ssize_t bonding_show_ad_actor_system(struct device *d,
					    struct device_attribute *attr,
					    char *buf)
{
	struct bonding *bond = to_bond(d);

	if (BOND_MODE(bond) == BOND_MODE_8023AD && capable(CAP_NET_ADMIN))
		return sprintf(buf, "%pM\n", bond->params.ad_actor_system);

	return 0;
}
