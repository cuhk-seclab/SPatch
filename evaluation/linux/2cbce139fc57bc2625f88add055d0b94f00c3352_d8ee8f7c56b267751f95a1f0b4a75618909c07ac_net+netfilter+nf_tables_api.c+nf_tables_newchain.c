static int nf_tables_newchain(struct sock *nlsk, struct sk_buff *skb,
			      const struct nlmsghdr *nlh,
			      const struct nlattr * const nla[])
{
	const struct nfgenmsg *nfmsg = nlmsg_data(nlh);
	const struct nlattr * uninitialized_var(name);
	struct nft_af_info *afi;
	struct nft_table *table;
	struct nft_chain *chain;
	struct nft_base_chain *basechain = NULL;
	struct nlattr *ha[NFTA_HOOK_MAX + 1];
	struct net *net = sock_net(skb->sk);
	int family = nfmsg->nfgen_family;
	struct net_device *dev = NULL;
	u8 policy = NF_ACCEPT;
	u64 handle = 0;
	unsigned int i;
	struct nft_stats __percpu *stats;
	int err;
	bool create;
	struct nft_ctx ctx;

	create = nlh->nlmsg_flags & NLM_F_CREATE ? true : false;

	afi = nf_tables_afinfo_lookup(net, family, true);
	if (IS_ERR(afi))
		return PTR_ERR(afi);

	table = nf_tables_table_lookup(afi, nla[NFTA_CHAIN_TABLE]);
	if (IS_ERR(table))
		return PTR_ERR(table);

	chain = NULL;
	name = nla[NFTA_CHAIN_NAME];

	if (nla[NFTA_CHAIN_HANDLE]) {
		handle = be64_to_cpu(nla_get_be64(nla[NFTA_CHAIN_HANDLE]));
		chain = nf_tables_chain_lookup_byhandle(table, handle);
		if (IS_ERR(chain))
			return PTR_ERR(chain);
	} else {
		chain = nf_tables_chain_lookup(table, name);
		if (IS_ERR(chain)) {
			if (PTR_ERR(chain) != -ENOENT)
				return PTR_ERR(chain);
			chain = NULL;
		}
	}

	if (nla[NFTA_CHAIN_POLICY]) {
		if ((chain != NULL &&
		    !(chain->flags & NFT_BASE_CHAIN)))
			return -EOPNOTSUPP;

		if (chain == NULL &&
		    nla[NFTA_CHAIN_HOOK] == NULL)
			return -EOPNOTSUPP;

		policy = ntohl(nla_get_be32(nla[NFTA_CHAIN_POLICY]));
		switch (policy) {
		case NF_DROP:
		case NF_ACCEPT:
			break;
		default:
			return -EINVAL;
		}
	}

	if (chain != NULL) {
		struct nft_stats *stats = NULL;
		struct nft_trans *trans;

		if (chain->flags & NFT_CHAIN_INACTIVE)
			return -ENOENT;
		if (nlh->nlmsg_flags & NLM_F_EXCL)
			return -EEXIST;
		if (nlh->nlmsg_flags & NLM_F_REPLACE)
			return -EOPNOTSUPP;

		if (nla[NFTA_CHAIN_HANDLE] && name &&
		    !IS_ERR(nf_tables_chain_lookup(table, nla[NFTA_CHAIN_NAME])))
			return -EEXIST;

		if (nla[NFTA_CHAIN_COUNTERS]) {
			if (!(chain->flags & NFT_BASE_CHAIN))
				return -EOPNOTSUPP;

			stats = nft_stats_alloc(nla[NFTA_CHAIN_COUNTERS]);
			if (IS_ERR(stats))
				return PTR_ERR(stats);
		}

		nft_ctx_init(&ctx, skb, nlh, afi, table, chain, nla);
		trans = nft_trans_alloc(&ctx, NFT_MSG_NEWCHAIN,
					sizeof(struct nft_trans_chain));
		if (trans == NULL) {
			free_percpu(stats);
			return -ENOMEM;
		}

		nft_trans_chain_stats(trans) = stats;
		nft_trans_chain_update(trans) = true;

		if (nla[NFTA_CHAIN_POLICY])
			nft_trans_chain_policy(trans) = policy;
		else
			nft_trans_chain_policy(trans) = -1;

		if (nla[NFTA_CHAIN_HANDLE] && name) {
			nla_strlcpy(nft_trans_chain_name(trans), name,
				    NFT_CHAIN_MAXNAMELEN);
		}
		list_add_tail(&trans->list, &net->nft.commit_list);
		return 0;
	}

	if (table->use == UINT_MAX)
		return -EOVERFLOW;

	if (nla[NFTA_CHAIN_HOOK]) {
		const struct nf_chain_type *type;
		struct nf_hook_ops *ops;
		nf_hookfn *hookfn;
		u32 hooknum, priority;

		type = chain_type[family][NFT_CHAIN_T_DEFAULT];
		if (nla[NFTA_CHAIN_TYPE]) {
			type = nf_tables_chain_type_lookup(afi,
							   nla[NFTA_CHAIN_TYPE],
							   create);
			if (IS_ERR(type))
				return PTR_ERR(type);
		}

		err = nla_parse_nested(ha, NFTA_HOOK_MAX, nla[NFTA_CHAIN_HOOK],
				       nft_hook_policy);
		if (err < 0)
			return err;
		if (ha[NFTA_HOOK_HOOKNUM] == NULL ||
		    ha[NFTA_HOOK_PRIORITY] == NULL)
			return -EINVAL;

		hooknum = ntohl(nla_get_be32(ha[NFTA_HOOK_HOOKNUM]));
		if (hooknum >= afi->nhooks)
			return -EINVAL;
		priority = ntohl(nla_get_be32(ha[NFTA_HOOK_PRIORITY]));

		if (!(type->hook_mask & (1 << hooknum)))
			return -EOPNOTSUPP;
		if (!try_module_get(type->owner))
			return -ENOENT;
		hookfn = type->hooks[hooknum];

		if (afi->flags & NFT_AF_NEEDS_DEV) {
			char ifname[IFNAMSIZ];

			if (!ha[NFTA_HOOK_DEV]) {
				module_put(type->owner);
				return -EOPNOTSUPP;
			}

			nla_strlcpy(ifname, ha[NFTA_HOOK_DEV], IFNAMSIZ);
			dev = dev_get_by_name(net, ifname);
			if (!dev) {
				module_put(type->owner);
				return -ENOENT;
			}
		} else if (ha[NFTA_HOOK_DEV]) {
			module_put(type->owner);
			return -EOPNOTSUPP;
		}

		basechain = kzalloc(sizeof(*basechain), GFP_KERNEL);
		if (basechain == NULL) {
			module_put(type->owner);
			if (dev != NULL)
				dev_put(dev);
			return -ENOMEM;
		}

		if (dev != NULL)
			strncpy(basechain->dev_name, dev->name, IFNAMSIZ);

		if (nla[NFTA_CHAIN_COUNTERS]) {
			stats = nft_stats_alloc(nla[NFTA_CHAIN_COUNTERS]);
			if (IS_ERR(stats)) {
				module_put(type->owner);
				kfree(basechain);
				if (dev != NULL)
					dev_put(dev);
				return PTR_ERR(stats);
			}
			basechain->stats = stats;
		} else {
			stats = netdev_alloc_pcpu_stats(struct nft_stats);
			if (stats == NULL) {
				module_put(type->owner);
				kfree(basechain);
				if (dev != NULL)
					dev_put(dev);
				return -ENOMEM;
			}
			rcu_assign_pointer(basechain->stats, stats);
		}

		write_pnet(&basechain->pnet, net);
		basechain->type = type;
		chain = &basechain->chain;

		for (i = 0; i < afi->nops; i++) {
			ops = &basechain->ops[i];
			ops->pf		= family;
			ops->owner	= afi->owner;
			ops->hooknum	= hooknum;
			ops->priority	= priority;
			ops->priv	= chain;
			ops->hook	= afi->hooks[ops->hooknum];
			ops->dev	= dev;
			if (hookfn)
				ops->hook = hookfn;
			if (afi->hook_ops_init)
				afi->hook_ops_init(ops, i);
		}

		chain->flags |= NFT_BASE_CHAIN;
		basechain->policy = policy;
	} else {
		chain = kzalloc(sizeof(*chain), GFP_KERNEL);
		if (chain == NULL)
			return -ENOMEM;
	}

	INIT_LIST_HEAD(&chain->rules);
	chain->handle = nf_tables_alloc_handle(table);
	chain->table = table;
	nla_strlcpy(chain->name, name, NFT_CHAIN_MAXNAMELEN);

	err = nf_tables_register_hooks(table, chain, afi->nops);
	if (err < 0)
		goto err1;

	nft_ctx_init(&ctx, skb, nlh, afi, table, chain, nla);
	err = nft_trans_chain_add(&ctx, NFT_MSG_NEWCHAIN);
	if (err < 0)
		goto err2;

	table->use++;
	list_add_tail_rcu(&chain->list, &table->chains);
	return 0;
err2:
	nf_tables_unregister_hooks(table, chain, afi->nops);
err1:
	nf_tables_chain_destroy(chain);
	return err;
}
