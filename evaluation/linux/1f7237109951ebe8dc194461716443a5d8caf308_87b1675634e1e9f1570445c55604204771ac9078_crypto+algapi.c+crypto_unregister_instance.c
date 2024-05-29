int crypto_unregister_instance(struct crypto_alg *alg)
{
	struct crypto_instance *inst = (void *)alg;
	LIST_HEAD(list);

	if (!(alg->cra_flags & CRYPTO_ALG_INSTANCE))
		return -EINVAL;

	down_write(&crypto_alg_sem);

	crypto_remove_spawns(alg, &list, NULL);
	crypto_remove_instance(inst, &list);

	up_write(&crypto_alg_sem);

	crypto_remove_final(&list);

	return 0;
}
