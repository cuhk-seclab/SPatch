static int
bitmap_ipmac_uadt(struct ip_set *set, struct nlattr *tb[],
		  enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	const struct bitmap_ipmac *map = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct bitmap_ipmac_adt_elem e = { .id = 0 };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	u32 ip = 0;
	int ret = 0;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	if (unlikely(!tb[IPSET_ATTR_IP]))
		return -IPSET_ERR_PROTOCOL;

	ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP], &ip);
	if (ret)
		return ret;

	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;

	if (ip < map->first_ip || ip > map->last_ip)
		return -IPSET_ERR_BITMAP_RANGE;

	e.id = ip_to_id(map, ip);
	if (tb[IPSET_ATTR_ETHER])
		e.ether = nla_data(tb[IPSET_ATTR_ETHER]);
	else
		e.ether = NULL;

	ret = adtfn(set, &e, &ext, &ext, flags);

	return ip_set_eexist(ret, flags) ? 0 : ret;
}
