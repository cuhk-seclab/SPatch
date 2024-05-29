static int
hash_ipmark4_uadt(struct ip_set *set, struct nlattr *tb[],
		  enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	const struct hash_ipmark *h = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct hash_ipmark4_elem e = { };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	u32 ip, ip_to = 0;
	int ret;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	if (unlikely(!tb[IPSET_ATTR_IP] ||
		     !ip_set_attr_netorder(tb, IPSET_ATTR_MARK)))
		return -IPSET_ERR_PROTOCOL;

	ret = ip_set_get_ipaddr4(tb[IPSET_ATTR_IP], &e.ip);
	if (ret)
		return ret;

	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;

	e.mark = ntohl(nla_get_be32(tb[IPSET_ATTR_MARK]));
	e.mark &= h->markmask;

	if (adt == IPSET_TEST ||
	    !(tb[IPSET_ATTR_IP_TO] || tb[IPSET_ATTR_CIDR])) {
		ret = adtfn(set, &e, &ext, &ext, flags);
		return ip_set_eexist(ret, flags) ? 0 : ret;
	}

	ip_to = ip = ntohl(e.ip);
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

	if (retried)
		ip = ntohl(h->next.ip);
	for (; !before(ip_to, ip); ip++) {
		e.ip = htonl(ip);
		ret = adtfn(set, &e, &ext, &ext, flags);

		if (ret && !ip_set_eexist(ret, flags))
			return ret;
		else
			ret = 0;
	}
	return ret;
}
