static struct mac802154_llsec_key*
llsec_key_alloc(const struct ieee802154_llsec_key *template)
{
	const int authsizes[3] = { 4, 8, 16 };
	struct mac802154_llsec_key *key;
	int i;

	key = kzalloc(sizeof(*key), GFP_KERNEL);
	if (!key)
		return NULL;

	kref_init(&key->ref);
	key->key = *template;

	BUILD_BUG_ON(ARRAY_SIZE(authsizes) != ARRAY_SIZE(key->tfm));

	for (i = 0; i < ARRAY_SIZE(key->tfm); i++) {
		key->tfm[i] = crypto_alloc_aead("ccm(aes)", 0,
						CRYPTO_ALG_ASYNC);
		if (!key->tfm[i])
			goto err_tfm;
		if (crypto_aead_setkey(key->tfm[i], template->key,
				       IEEE802154_LLSEC_KEY_SIZE))
			goto err_tfm;
		if (crypto_aead_setauthsize(key->tfm[i], authsizes[i]))
			goto err_tfm;
	}

	key->tfm0 = crypto_alloc_blkcipher("ctr(aes)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(key->tfm0))
		goto err_tfm;

	if (crypto_blkcipher_setkey(key->tfm0, template->key,
				    IEEE802154_LLSEC_KEY_SIZE))
		goto err_tfm0;

	return key;

err_tfm0:
	crypto_free_blkcipher(key->tfm0);
err_tfm:
	for (i = 0; i < ARRAY_SIZE(key->tfm); i++)
		if (key->tfm[i])
			crypto_free_aead(key->tfm[i]);

	kfree(key);
	return NULL;
}
