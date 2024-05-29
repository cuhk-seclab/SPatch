static int skcipher_alloc_sgl(struct sock *sk)
{
	struct alg_sock *ask = alg_sk(sk);
	struct skcipher_ctx *ctx = ask->private;
	struct skcipher_sg_list *sgl;
	struct scatterlist *sg = NULL;

	sgl = list_entry(ctx->tsgl.prev, struct skcipher_sg_list, list);
	if (!list_empty(&ctx->tsgl))
		sg = sgl->sg;

	if (!sg || sgl->cur >= MAX_SGL_ENTS) {
		sgl = sock_kmalloc(sk, sizeof(*sgl) +
				       sizeof(sgl->sg[0]) * (MAX_SGL_ENTS + 1),
				   GFP_KERNEL);
		if (!sgl)
			return -ENOMEM;

		sg_init_table(sgl->sg, MAX_SGL_ENTS + 1);
		sgl->cur = 0;

		if (sg)
			sg_chain(sg, MAX_SGL_ENTS + 1, sgl->sg);

		list_add_tail(&sgl->list, &ctx->tsgl);
	}

	return 0;
}
