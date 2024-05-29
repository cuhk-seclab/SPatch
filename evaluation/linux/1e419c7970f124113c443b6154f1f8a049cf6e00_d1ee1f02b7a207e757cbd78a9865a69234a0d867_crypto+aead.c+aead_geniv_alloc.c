struct aead_instance *aead_geniv_alloc(struct crypto_template *tmpl,
				       struct rtattr **tb, u32 type, u32 mask)
{
	const char *name;
	struct crypto_aead_spawn *spawn;
	struct crypto_attr_type *algt;
	struct aead_instance *inst;
	struct aead_alg *alg;
	unsigned int ivsize;
	unsigned int maxauthsize;
	int err;

	algt = crypto_get_attr_type(tb);
	if (IS_ERR(algt))
		return ERR_CAST(algt);

	if ((algt->type ^ (CRYPTO_ALG_TYPE_AEAD | CRYPTO_ALG_GENIV)) &
	    algt->mask)
		return ERR_PTR(-EINVAL);

	name = crypto_attr_alg_name(tb[1]);
	if (IS_ERR(name))
		return ERR_CAST(name);

	inst = kzalloc(sizeof(*inst) + sizeof(*spawn), GFP_KERNEL);
	if (!inst)
		return ERR_PTR(-ENOMEM);

	spawn = aead_instance_ctx(inst);

	/* Ignore async algorithms if necessary. */
	mask |= crypto_requires_sync(algt->type, algt->mask);

	crypto_set_aead_spawn(spawn, aead_crypto_instance(inst));
	err = crypto_grab_nivaead(spawn, name, type, mask);
	if (err)
		goto err_free_inst;

	alg = crypto_spawn_aead_alg(spawn);

	ivsize = crypto_aead_alg_ivsize(alg);
	maxauthsize = crypto_aead_alg_maxauthsize(alg);

	err = -EINVAL;
	if (!ivsize)
		goto err_drop_alg;

	/*
	 * This is only true if we're constructing an algorithm with its
	 * default IV generator.  For the default generator we elide the
	 * template name and double-check the IV generator.
	 */
	if (algt->mask & CRYPTO_ALG_GENIV) {
		if (!alg->base.cra_aead.encrypt)
			goto err_drop_alg;
		if (strcmp(tmpl->name, alg->base.cra_aead.geniv))
			goto err_drop_alg;

		memcpy(inst->alg.base.cra_name, alg->base.cra_name,
		       CRYPTO_MAX_ALG_NAME);
		memcpy(inst->alg.base.cra_driver_name,
		       alg->base.cra_driver_name, CRYPTO_MAX_ALG_NAME);

		inst->alg.base.cra_flags = CRYPTO_ALG_TYPE_AEAD |
					   CRYPTO_ALG_GENIV;
		inst->alg.base.cra_flags |= alg->base.cra_flags &
					    CRYPTO_ALG_ASYNC;
		inst->alg.base.cra_priority = alg->base.cra_priority;
		inst->alg.base.cra_blocksize = alg->base.cra_blocksize;
		inst->alg.base.cra_alignmask = alg->base.cra_alignmask;
		inst->alg.base.cra_type = &crypto_aead_type;

		inst->alg.base.cra_aead.ivsize = ivsize;
		inst->alg.base.cra_aead.maxauthsize = maxauthsize;

		inst->alg.base.cra_aead.setkey = alg->base.cra_aead.setkey;
		inst->alg.base.cra_aead.setauthsize =
			alg->base.cra_aead.setauthsize;
		inst->alg.base.cra_aead.encrypt = alg->base.cra_aead.encrypt;
		inst->alg.base.cra_aead.decrypt = alg->base.cra_aead.decrypt;

		goto out;
	}

	err = -ENAMETOOLONG;
	if (snprintf(inst->alg.base.cra_name, CRYPTO_MAX_ALG_NAME,
		     "%s(%s)", tmpl->name, alg->base.cra_name) >=
	    CRYPTO_MAX_ALG_NAME)
		goto err_drop_alg;
	if (snprintf(inst->alg.base.cra_driver_name, CRYPTO_MAX_ALG_NAME,
		     "%s(%s)", tmpl->name, alg->base.cra_driver_name) >=
	    CRYPTO_MAX_ALG_NAME)
		goto err_drop_alg;

	inst->alg.base.cra_flags = alg->base.cra_flags & CRYPTO_ALG_ASYNC;
	inst->alg.base.cra_priority = alg->base.cra_priority;
	inst->alg.base.cra_blocksize = alg->base.cra_blocksize;
	inst->alg.base.cra_alignmask = alg->base.cra_alignmask;

	inst->alg.ivsize = ivsize;
	inst->alg.maxauthsize = maxauthsize;

out:
	return inst;

err_drop_alg:
	crypto_drop_aead(spawn);
err_free_inst:
	kfree(inst);
	inst = ERR_PTR(err);
	goto out;
}
