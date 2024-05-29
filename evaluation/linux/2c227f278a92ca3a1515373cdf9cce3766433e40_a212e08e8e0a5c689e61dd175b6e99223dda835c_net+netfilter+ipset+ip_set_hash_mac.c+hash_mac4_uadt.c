static int
hash_mac4_uadt(struct ip_set *set, struct nlattr *tb[],
	       enum ipset_adt adt, u32 *lineno, u32 flags, bool retried)
{
	ipset_adtfn adtfn = set->variant->adt[adt];
	struct hash_mac4_elem e = { { .foo[0] = 0, .foo[1] = 0 } };
	struct ip_set_ext ext = IP_SET_INIT_UEXT(set);
	int ret;

	if (tb[IPSET_ATTR_LINENO])
		*lineno = nla_get_u32(tb[IPSET_ATTR_LINENO]);

	if (unlikely(!tb[IPSET_ATTR_ETHER]))
		return -IPSET_ERR_PROTOCOL;

	ret = ip_set_get_extensions(set, tb, &ext);
	if (ret)
		return ret;
	memcpy(e.ether, nla_data(tb[IPSET_ATTR_ETHER]), ETH_ALEN);
	if (memcmp(e.ether, invalid_ether, ETH_ALEN) == 0)
		return -IPSET_ERR_HASH_ELEM;

	return adtfn(set, &e, &ext, &ext, flags);
}
