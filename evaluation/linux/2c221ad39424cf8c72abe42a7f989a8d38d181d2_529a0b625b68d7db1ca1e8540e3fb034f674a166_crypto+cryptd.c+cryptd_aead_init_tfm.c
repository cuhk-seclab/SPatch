static int cryptd_aead_init_tfm(struct crypto_tfm *tfm)
{
	struct crypto_instance *inst = crypto_tfm_alg_instance(tfm);
	struct aead_instance_ctx *ictx = crypto_instance_ctx(inst);
	struct crypto_aead_spawn *spawn = &ictx->aead_spawn;
	struct cryptd_aead_ctx *ctx = crypto_tfm_ctx(tfm);
	struct crypto_aead *cipher;

	cipher = crypto_spawn_aead(spawn);
	if (IS_ERR(cipher))
		return PTR_ERR(cipher);

	crypto_aead_set_flags(cipher, CRYPTO_TFM_REQ_MAY_SLEEP);
	ctx->child = cipher;
	crypto_aead_set_reqsize(__crypto_aead_cast(tfm),
				sizeof(struct cryptd_aead_request_ctx));
	return 0;
}
