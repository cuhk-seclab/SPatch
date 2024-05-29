static int
hash_ip4_uadt(struct ip_set *set, struct nlattr *tb[],
	      enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	const struct hash_ip *h = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct hash_ip4_elem e = { 0 };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	u32 ip = 0, ip_to = 0, hosts;
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

	ip &= ip_set_hostmask(h->netmask);

	if (adt == IPSET_TEST) {
		e.ip = htonl(ip);
		if (e.ip == 0)
			return -IPSET_ERR_HASH_ELEM;
		return adtfn(set, &e, &ext, &ext, flags);
	}

	ip_to = ip;
	if (tb[IPSET_ATTR_IP_TO]) {
		ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP_TO], &ip_to);
		if (ret)
			return ret;
		if (ip > ip_to)
			swap(ip, ip_to);
	} else if (tb[IPSET_ATTR_CIDR]) {
		u8 cidr = nla_get_u8(tb[IPSET_ATTR_CIDR]);

		if (!cidr || cidr > HOST_MASK)
			return -IPSET_ERR_INVALID_CIDR;
		ip_set_mask_from_to(ip, ip_to, cidr);
	}

	hosts = h->netmask == 32 ? 1 : 2 << (32 - h->netmask - 1);

	if (retried)
		ip = ntohl(h->next.ip);
	for (; !before(ip_to, ip); ip += hosts) {
		e.ip = htonl(ip);
		if (e.ip == 0)
			return -IPSET_ERR_HASH_ELEM;
		ret = adtfn(set, &e, &ext, &ext, flags);

		if (ret && !ip_set_eexist(ret, flags))
			return ret;
		else
			ret = 0;
	}
	return ret;
}
