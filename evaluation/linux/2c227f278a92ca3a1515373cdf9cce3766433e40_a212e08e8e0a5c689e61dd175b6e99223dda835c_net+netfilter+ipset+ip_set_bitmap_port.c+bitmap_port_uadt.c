static int
bitmap_port_uadt(struct ip_set *set, struct nlattr *tb[],
		 enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	struct bitmap_port *map = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct bitmap_port_adt_elem e = { .id = 0 };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	u32 port;	/* wraparound */
	u16 port_to;
	int ret = 0;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	if (unlikely(!ip_set_attr_netorder(tb, IPSET_ATTR_PORT) ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_PORT_TO)))
		return -IPSET_ERR_PROTOCOL;

	port = ip_set_get_h16(tb[IPSET_ATTR_PORT]);
	if (port < map->first_port || port > map->last_port)
		return -IPSET_ERR_BITMAP_RANGE;
	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;

	if (adt == IPSET_TEST) {
		e.id = port_to_id(map, port);
		return adtfn(set, &e, &ext, &ext, flags);
	}

	if (tb[IPSET_ATTR_PORT_TO]) {
		port_to = ip_set_get_h16(tb[IPSET_ATTR_PORT_TO]);
		if (port > port_to) {
			swap(port, port_to);
			if (port < map->first_port)
				return -IPSET_ERR_BITMAP_RANGE;
		}
	} else
		port_to = port;

	if (port_to > map->last_port)
		return -IPSET_ERR_BITMAP_RANGE;

	for (; port <= port_to; port++) {
		e.id = port_to_id(map, port);
		ret = adtfn(set, &e, &ext, &ext, flags);

		if (ret && !ip_set_eexist(ret, flags))
			return ret;
		else
			ret = 0;
	}
	return ret;
}
