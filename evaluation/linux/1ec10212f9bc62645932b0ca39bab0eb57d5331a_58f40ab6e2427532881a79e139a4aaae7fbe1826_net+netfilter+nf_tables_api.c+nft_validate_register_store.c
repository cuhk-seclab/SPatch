int nft_validate_register_store(const struct nft_ctx *ctx,
				enum nft_registers reg,
				const struct nft_data *data,
				enum nft_data_types type, unsigned int len)
{
	int err;

	switch (reg) {
	case NFT_REG_VERDICT:
		if (type != NFT_DATA_VERDICT)
			return -EINVAL;

		if (data != NULL &&
		    (data->verdict == NFT_GOTO || data->verdict == NFT_JUMP)) {
			err = nf_tables_check_loops(ctx, data->chain);
			if (err < 0)
				return err;

			if (ctx->chain->level + 1 > data->chain->level) {
				if (ctx->chain->level + 1 == NFT_JUMP_STACK_SIZE)
					return -EMLINK;
				data->chain->level = ctx->chain->level + 1;
			}
		}

		return 0;
	default:
		if (len == 0)
			return -EINVAL;
		if (len > FIELD_SIZEOF(struct nft_data, data))
			return -ERANGE;
		if (data != NULL && type != NFT_DATA_VALUE)
			return -EINVAL;
		return 0;
	}
}
