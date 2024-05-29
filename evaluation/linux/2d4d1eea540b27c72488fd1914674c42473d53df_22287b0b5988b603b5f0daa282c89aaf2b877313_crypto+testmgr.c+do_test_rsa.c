static int do_test_rsa(struct crypto_akcipher *tfm,
		       struct akcipher_testvec *vecs)
{
	struct akcipher_request *req;
	void *outbuf_enc = NULL;
	void *outbuf_dec = NULL;
	struct tcrypt_result result;
	unsigned int out_len_max, out_len = 0;
	int err = -ENOMEM;
	struct scatterlist src, dst, src_tab[2];

	req = akcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		return err;

	init_completion(&result.completion);

	if (vecs->public_key_vec)
		err = crypto_akcipher_set_pub_key(tfm, vecs->key,
						  vecs->key_len);
	else
		err = crypto_akcipher_set_priv_key(tfm, vecs->key,
						   vecs->key_len);
	if (err)
		goto free_req;

	out_len_max = crypto_akcipher_maxsize(tfm);
	outbuf_enc = kzalloc(out_len_max, GFP_KERNEL);
	if (!outbuf_enc)
		goto free_req;

	sg_init_table(src_tab, 2);
	sg_set_buf(&src_tab[0], vecs->m, 8);
	sg_set_buf(&src_tab[1], vecs->m + 8, vecs->m_size - 8);
	sg_init_one(&dst, outbuf_enc, out_len_max);
	akcipher_request_set_crypt(req, src_tab, &dst, vecs->m_size,
				   out_len_max);
	akcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				      tcrypt_complete, &result);

	/* Run RSA encrypt - c = m^e mod n;*/
	err = wait_async_op(&result, crypto_akcipher_encrypt(req));
	if (err) {
		pr_err("alg: rsa: encrypt test failed. err %d\n", err);
		goto free_all;
	}
	if (req->dst_len != vecs->c_size) {
		pr_err("alg: rsa: encrypt test failed. Invalid output len\n");
		err = -EINVAL;
		goto free_all;
	}
	/* verify that encrypted message is equal to expected */
	if (memcmp(vecs->c, sg_virt(req->dst), vecs->c_size)) {
		pr_err("alg: rsa: encrypt test failed. Invalid output\n");
		err = -EINVAL;
		goto free_all;
	}
	/* Don't invoke decrypt for vectors with public key */
	if (vecs->public_key_vec) {
		err = 0;
		goto free_all;
	}
	outbuf_dec = kzalloc(out_len_max, GFP_KERNEL);
	if (!outbuf_dec) {
		err = -ENOMEM;
		goto free_all;
	}
	sg_init_one(&src, vecs->c, vecs->c_size);
	sg_init_one(&dst, outbuf_dec, out_len_max);
	init_completion(&result.completion);
	akcipher_request_set_crypt(req, &src, &dst, vecs->c_size, out_len_max);

	/* Run RSA decrypt - m = c^d mod n;*/
	err = wait_async_op(&result, crypto_akcipher_decrypt(req));
	if (err) {
		pr_err("alg: rsa: decrypt test failed. err %d\n", err);
		goto free_all;
	}
	out_len = req->dst_len;
	if (out_len != vecs->m_size) {
		pr_err("alg: rsa: decrypt test failed. Invalid output len\n");
		err = -EINVAL;
		goto free_all;
	}
	/* verify that decrypted message is equal to the original msg */
	if (memcmp(vecs->m, outbuf_dec, vecs->m_size)) {
		pr_err("alg: rsa: decrypt test failed. Invalid output\n");
		err = -EINVAL;
	}
free_all:
	kfree(outbuf_dec);
	kfree(outbuf_enc);
free_req:
	akcipher_request_free(req);
	return err;
}
