static int
hash_netnet6_uadt(struct ip_set *set, struct nlattr *tb[],
	       enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct hash_netnet6_elem e = { };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	int ret;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	e.cidr[0] = e.cidr[1] = HOST_MASK;
	if (unlikely(!tb[IPSET_ATTR_IP] || !tb[IPSET_ATTR_IP2] ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_CADT_FLAGS)))
		return -IPSET_ERR_PROTOCOL;
	if (unlikely(tb[IPSET_ATTR_IP_TO] || tb[IPSET_ATTR_IP2_TO]))
		return -IPSET_ERR_HASH_RANGE_UNSUPPORTED;

	ret = ip_set_get_ipaddr6(tb[IPSET_ATTR_IP], &e.ip[0]);
	if (ret)
		return ret;

	ret = ip_set_get_ipaddr6(tb[IPSET_ATTR_IP2], &e.ip[1]);
	if (ret)
		return ret;

	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;

	if (tb[IPSET_ATTR_CIDR])
		e.cidr[0] = nla_get_u8(tb[IPSET_ATTR_CIDR]);

	if (tb[IPSET_ATTR_CIDR2])
		e.cidr[1] = nla_get_u8(tb[IPSET_ATTR_CIDR2]);

	if (!e.cidr[0] || e.cidr[0] > HOST_MASK || !e.cidr[1] ||
	    e.cidr[1] > HOST_MASK)
		return -IPSET_ERR_INVALID_CIDR;

	ip6_netmask(&e.ip[0], e.cidr[0]);
	ip6_netmask(&e.ip[1], e.cidr[1]);

	if (tb[IPSET_ATTR_CADT_FLAGS]) {
		u32 cadt_flags = ip_set_get_h32(tb[IPSET_ATTR_CADT_FLAGS]);
		if (cadt_flags & IPSET_FLAG_NOMATCH)
			flags |= (IPSET_FLAG_NOMATCH << 16);
	}

	ret = adtfn(set, &e, &ext, &ext, flags);

	return ip_set_enomatch(ret, flags, adt, set) ? -ret :
	       ip_set_eexist(ret, flags) ? 0 : ret;
}
