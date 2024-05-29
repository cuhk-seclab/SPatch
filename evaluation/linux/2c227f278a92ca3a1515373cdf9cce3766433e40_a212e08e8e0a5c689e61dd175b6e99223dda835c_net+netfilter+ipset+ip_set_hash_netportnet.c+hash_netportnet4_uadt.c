static int
hash_netportnet4_uadt(struct ip_set *set, struct nlattr *tb[],
		     enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	const struct hash_netportnet *h = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct hash_netportnet4_elem e = { };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	u32 ip = 0, ip_to = 0, ip_last, p = 0, port, port_to;
	u32 ip2_from = 0, ip2_to = 0, ip2_last, ip2;
	bool with_ports = false;
	u8 cidr, cidr2;
	int ret;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	e.cidr[0] = e.cidr[1] = HOST_MASK;
	if (unlikely(!tb[IPSET_ATTR_IP] || !tb[IPSET_ATTR_IP2] ||
		     !ip_set_attr_netorder(tb, IPSET_ATTR_PORT) ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_PORT_TO) ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_CADT_FLAGS)))
		return -IPSET_ERR_PROTOCOL;

	ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP], &ip);
	if (ret)
		return ret;

	ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP2], &ip2_from);
	if (ret)
		return ret;

	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;

	if (tb[IPSET_ATTR_CIDR]) {
		cidr = nla_get_u8(tb[IPSET_ATTR_CIDR]);
		if (!cidr || cidr > HOST_MASK)
			return -IPSET_ERR_INVALID_CIDR;
		e.cidr[0] = cidr;
	}

	if (tb[IPSET_ATTR_CIDR2]) {
		cidr = nla_get_u8(tb[IPSET_ATTR_CIDR2]);
		if (!cidr || cidr > HOST_MASK)
			return -IPSET_ERR_INVALID_CIDR;
		e.cidr[1] = cidr;
	}

	e.port = nla_get_be16(tb[IPSET_ATTR_PORT]);

	if (tb[IPSET_ATTR_PROTO]) {
		e.proto = nla_get_u8(tb[IPSET_ATTR_PROTO]);
		with_ports = ip_set_proto_with_ports(e.proto);

		if (e.proto == 0)
			return -IPSET_ERR_INVALID_PROTO;
	} else
		return -IPSET_ERR_MISSING_PROTO;

	if (!(with_ports || e.proto == IPPROTO_ICMP))
		e.port = 0;

	if (tb[IPSET_ATTR_CADT_FLAGS]) {
		u32 cadt_flags = ip_set_get_h32(tb[IPSET_ATTR_CADT_FLAGS]);
		if (cadt_flags & IPSET_FLAG_NOMATCH)
			flags |= (IPSET_FLAG_NOMATCH << 16);
	}

	with_ports = with_ports && tb[IPSET_ATTR_PORT_TO];
	if (adt == IPSET_TEST ||
	    !(tb[IPSET_ATTR_IP_TO] || with_ports || tb[IPSET_ATTR_IP2_TO])) {
		e.ip[0] = htonl(ip & ip_set_hostmask(e.cidr[0]));
		e.ip[1] = htonl(ip2_from & ip_set_hostmask(e.cidr[1]));
		ret = adtfn(set, &e, &ext, &ext, flags);
		return ip_set_enomatch(ret, flags, adt, set) ? -ret :
		       ip_set_eexist(ret, flags) ? 0 : ret;
	}

	ip_to = ip;
	if (tb[IPSET_ATTR_IP_TO]) {
		ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP_TO], &ip_to);
		if (ret)
			return ret;
		if (ip > ip_to)
			swap(ip, ip_to);
		if (unlikely(ip + UINT_MAX == ip_to))
			return -IPSET_ERR_HASH_RANGE;
	} else
		ip_set_mask_from_to(ip, ip_to, e.cidr[0]);

	port_to = port = ntohs(e.port);
	if (tb[IPSET_ATTR_PORT_TO]) {
		port_to = ip_set_get_h16(tb[IPSET_ATTR_PORT_TO]);
		if (port > port_to)
			swap(port, port_to);
	}

	ip2_to = ip2_from;
	if (tb[IPSET_ATTR_IP2_TO]) {
		ret = ip_set_get_hostipaddr4(tb[IPSET_ATTR_IP2_TO], &ip2_to);
		if (ret)
			return ret;
		if (ip2_from > ip2_to)
			swap(ip2_from, ip2_to);
		if (unlikely(ip2_from + UINT_MAX == ip2_to))
			return -IPSET_ERR_HASH_RANGE;
	} else
		ip_set_mask_from_to(ip2_from, ip2_to, e.cidr[1]);

	if (retried)
		ip = ntohl(h->next.ip[0]);

	while (!after(ip, ip_to)) {
		e.ip[0] = htonl(ip);
		ip_last = ip_set_range_to_cidr(ip, ip_to, &cidr);
		e.cidr[0] = cidr;
		p = retried && ip == ntohl(h->next.ip[0]) ? ntohs(h->next.port)
							  : port;
		for (; p <= port_to; p++) {
			e.port = htons(p);
			ip2 = (retried && ip == ntohl(h->next.ip[0]) &&
			       p == ntohs(h->next.port)) ? ntohl(h->next.ip[1])
							 : ip2_from;
			while (!after(ip2, ip2_to)) {
				e.ip[1] = htonl(ip2);
				ip2_last = ip_set_range_to_cidr(ip2, ip2_to,
								&cidr2);
				e.cidr[1] = cidr2;
				ret = adtfn(set, &e, &ext, &ext, flags);
				if (ret && !ip_set_eexist(ret, flags))
					return ret;
				else
					ret = 0;
				ip2 = ip2_last + 1;
			}
		}
		ip = ip_last + 1;
	}
	return ret;
}
