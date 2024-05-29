static int
bitmap_ip_uadt(struct ip_set *set, struct nlattr *tb[],
	       enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	struct bitmap_ip *map = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	u32 ip = 0, ip_to = 0;
	struct bitmap_ip_adt_elem e = { .id = 0 };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
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

	if (adt == IPSET_TEST) {
		e.id = ip_to_id(map, ip);
		return adtfn(set, &e, &ext, &ext, flags);
	}

	if (tb[IPSET_ATTR_IP_TO]) {
		ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP_TO], &ip_to);
		if (ret)
			return ret;
		if (ip > ip_to) {
			swap(ip, ip_to);
			if (ip < map->first_ip)
				return -IPSET_ERR_BITMAP_RANGE;
		}
	} else if (tb[IPSET_ATTR_CIDR]) {
		u8 cidr = nla_get_u8(tb[IPSET_ATTR_CIDR]);

		if (!cidr || cidr > HOST_MASK)
			return -IPSET_ERR_INVALID_CIDR;
		ip_set_mask_from_to(ip, ip_to, cidr);
	} else
		ip_to = ip;

	if (ip_to > map->last_ip)
		return -IPSET_ERR_BITMAP_RANGE;

	for (; !before(ip_to, ip); ip += map->hosts) {
		e.id = ip_to_id(map, ip);
		ret = adtfn(set, &e, &ext, &ext, flags);

		if (ret && !ip_set_eexist(ret, flags))
			return ret;
		else
			ret = 0;
	}
	return ret;
}
