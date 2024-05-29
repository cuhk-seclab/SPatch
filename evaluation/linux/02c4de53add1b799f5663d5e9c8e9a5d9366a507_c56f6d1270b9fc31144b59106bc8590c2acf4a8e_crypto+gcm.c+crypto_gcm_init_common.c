static void crypto_gcm_init_common(struct aead_request *req)
{
	struct crypto_gcm_req_priv_ctx *pctx = crypto_gcm_reqctx(req);
	__be32 counter = cpu_to_be32(1);
	struct scatterlist *sg;

	memset(pctx->auth_tag, 0, sizeof(pctx->auth_tag));
	memcpy(pctx->iv, req->iv, 12);
	memcpy(pctx->iv + 12, &counter, 4);

	sg_init_table(pctx->src, 3);
	sg_set_buf(pctx->src, pctx->auth_tag, sizeof(pctx->auth_tag));
	sg = scatterwalk_ffwd(pctx->src + 1, req->src, req->assoclen);
	if (sg != pctx->src + 1)
		sg_chain(pctx->src, 2, sg);

	if (req->src != req->dst) {
		sg_init_table(pctx->dst, 3);
		sg_set_buf(pctx->dst, pctx->auth_tag, sizeof(pctx->auth_tag));
		sg = scatterwalk_ffwd(pctx->dst + 1, req->dst, req->assoclen);
		if (sg != pctx->dst + 1)
			sg_chain(pctx->dst, 2, sg);
	}
}
