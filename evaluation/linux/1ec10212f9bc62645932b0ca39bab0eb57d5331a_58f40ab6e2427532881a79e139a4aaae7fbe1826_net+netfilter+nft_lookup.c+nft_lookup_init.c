static int nft_lookup_init(const struct nft_ctx *ctx,
			   const struct nft_expr *expr,
			   const struct nlattr * const tb[])
{
	struct nft_lookup *priv = nft_expr_priv(expr);
	struct nft_set *set;
	int err;

	if (tb[NFTA_LOOKUP_SET] == NULL ||
	    tb[NFTA_LOOKUP_SREG] == NULL)
		return -EINVAL;

	set = nf_tables_set_lookup(ctx->table, tb[NFTA_LOOKUP_SET]);
	if (IS_ERR(set)) {
		if (tb[NFTA_LOOKUP_SET_ID]) {
			set = nf_tables_set_lookup_byid(ctx->net,
							tb[NFTA_LOOKUP_SET_ID]);
		}
		if (IS_ERR(set))
			return PTR_ERR(set);
	}

	priv->sreg = ntohl(nla_get_be32(tb[NFTA_LOOKUP_SREG]));
	err = nft_validate_input_register(priv->sreg);
	if (err < 0)
		return err;

	if (tb[NFTA_LOOKUP_DREG] != NULL) {
		if (!(set->flags & NFT_SET_MAP))
			return -EINVAL;

		priv->dreg = ntohl(nla_get_be32(tb[NFTA_LOOKUP_DREG]));
		err = nft_validate_output_register(priv->dreg);
		if (err < 0)
			return err;

		err = nft_validate_register_store(ctx, priv->dreg, NULL,
						  set->dtype, set->dlen);
		if (err < 0)
			return err;
	} else if (set->flags & NFT_SET_MAP)
		return -EINVAL;

	priv->binding.flags = set->flags & NFT_SET_MAP;

	err = nf_tables_bind_set(ctx, set, &priv->binding);
	if (err < 0)
		return err;

	priv->set = set;
	return 0;
}
