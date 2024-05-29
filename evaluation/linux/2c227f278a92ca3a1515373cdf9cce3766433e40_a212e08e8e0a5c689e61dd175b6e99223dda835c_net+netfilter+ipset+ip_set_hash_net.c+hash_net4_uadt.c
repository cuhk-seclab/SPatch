static int
hash_net4_uadt(struct ip_set *set, struct nlattr *tb[],
	       enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	const struct hash_net *h = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct hash_net4_elem e = { .cidr = HOST_MASK };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	u32 ip = 0, ip_to = 0, last;
	int ret;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	if (unlikely(!tb[IPSET_ATTR_IP] ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_CADT_FLAGS)))
		return -IPSET_ERR_PROTOCOL;

	ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP], &ip);
	if (ret)
		return ret;

	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;

	if (tb[IPSET_ATTR_CIDR]) {
		e.cidr = nla_get_u8(tb[IPSET_ATTR_CIDR]);
		if (!e.cidr || e.cidr > HOST_MASK)
			return -IPSET_ERR_INVALID_CIDR;
	}

	if (tb[IPSET_ATTR_CADT_FLAGS]) {
		u32 cadt_flags = ip_set_get_h32(tb[IPSET_ATTR_CADT_FLAGS]);
		if (cadt_flags & IPSET_FLAG_NOMATCH)
			flags |= (IPSET_FLAG_NOMATCH << 16);
	}

	if (adt == IPSET_TEST || !tb[IPSET_ATTR_IP_TO]) {
		e.ip = htonl(ip & ip_set_hostmask(e.cidr));
		ret = adtfn(set, &e, &ext, &ext, flags);
		return ip_set_enomatch(ret, flags, adt, set) ? -ret:
		       ip_set_eexist(ret, flags) ? 0 : ret;
	}

	ip_to = ip;
	if (tb[IPSET_ATTR_IP_TO]) {
		ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP_TO], &ip_to);
		if (ret)
			return ret;
		if (ip_to < ip)
			swap(ip, ip_to);
		if (ip + UINT_MAX == ip_to)
			return -IPSET_ERR_HASH_RANGE;
	}
	if (retried)
		ip = ntohl(h->next.ip);
	while (!after(ip, ip_to)) {
		e.ip = htonl(ip);
		last = ip_set_range_to_cidr(ip, ip_to, &e.cidr);
		ret = adtfn(set, &e, &ext, &ext, flags);
		if (ret && !ip_set_eexist(ret, flags))
			return ret;
		else
			ret = 0;
		ip = last + 1;
	}
	return ret;
}
