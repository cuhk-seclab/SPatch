static int
list_set_uadt(struct ip_set *set, struct nlattr *tb[],
	      enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	struct list_set *map = set->data;
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct set_adt_elem e = { .refid = IPSET_INVALID_ID };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	struct ip_set *s;
	int ret = 0;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	if (unlikely(!tb[IPSET_ATTR_NAME] ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_CADT_FLAGS)))
		return -IPSET_ERR_PROTOCOL;

	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;
	e.id = ip_set_get_byname(map->net, nla_data(tb[IPSET_ATTR_NAME]), &s);
	if (e.id == IPSET_INVALID_ID)
		return -IPSET_ERR_NAME;
	/* "Loop detection" */
	if (s->type->features & IPSET_TYPE_NAME) {
		ret = -IPSET_ERR_LOOP;
		goto finish;
	}

	if (tb[IPSET_ATTR_CADT_FLAGS]) {
		u32 f = ip_set_get_h32(tb[IPSET_ATTR_CADT_FLAGS]);
		e.before = f & IPSET_FLAG_BEFORE;
	}

	if (e.before && !tb[IPSET_ATTR_NAMEREF]) {
		ret = -IPSET_ERR_BEFORE;
		goto finish;
	}

	if (tb[IPSET_ATTR_NAMEREF]) {
		e.refid = ip_set_get_byname(map->net,
					    nla_data(tb[IPSET_ATTR_NAMEREF]),
					    &s);
		if (e.refid == IPSET_INVALID_ID) {
			ret = -IPSET_ERR_NAMEREF;
			goto finish;
		}
		if (!e.before)
			e.before = -1;
	}
	if (adt != IPSET_TEST && SET_WITH_TIMEOUT(set))
		set_cleanup_entries(set);

	ret = adtfn(set, &e, &ext, &ext, flags);

finish:
	if (e.refid != IPSET_INVALID_ID)
		ip_set_put_byindex(map->net, e.refid);
	if (adt != IPSET_ADD || ret)
		ip_set_put_byindex(map->net, e.id);

	return ip_set_eexist(ret, flags) ? 0 : ret;
}
