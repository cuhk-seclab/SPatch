static void test_ahash_speed(const char *algo, unsigned int secs,
			     struct hash_speed *speed)
{
	struct scatterlist sg[TVMEMSIZE];
	struct tcrypt_result tresult;
	struct ahash_request *req;
	struct crypto_ahash *tfm;
	char *output;
	int i, ret;

	tfm = crypto_alloc_ahash(algo, 0, 0);
	if (IS_ERR(tfm)) {
		pr_err("failed to load transform for %s: %ld\n",
		       algo, PTR_ERR(tfm));
		return;
	}

	printk(KERN_INFO "\ntesting speed of async %s (%s)\n", algo,
			get_driver_name(crypto_ahash, tfm));

	if (crypto_ahash_digestsize(tfm) > MAX_DIGEST_SIZE) {
		pr_err("digestsize(%u) > %d\n", crypto_ahash_digestsize(tfm),
		       MAX_DIGEST_SIZE);
		goto out;
	}

	test_hash_sg_init(sg);
	req = ahash_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		pr_err("ahash request allocation failure\n");
		goto out;
	}

	init_completion(&tresult.completion);
	ahash_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				   tcrypt_complete, &tresult);

	output = kmalloc(MAX_DIGEST_SIZE, GFP_KERNEL);
	if (!output)
		goto out_nomem;

	for (i = 0; speed[i].blen != 0; i++) {
		if (speed[i].blen > TVMEMSIZE * PAGE_SIZE) {
			pr_err("template (%u) too big for tvmem (%lu)\n",
			       speed[i].blen, TVMEMSIZE * PAGE_SIZE);
			break;
		}

		pr_info("test%3u "
			"(%5u byte blocks,%5u bytes per update,%4u updates): ",
			i, speed[i].blen, speed[i].plen, speed[i].blen / speed[i].plen);

		ahash_request_set_crypt(req, sg, output, speed[i].plen);

		if (secs)
			ret = test_ahash_jiffies(req, speed[i].blen,
						 speed[i].plen, output, secs);
		else
			ret = test_ahash_cycles(req, speed[i].blen,
						speed[i].plen, output);

		if (ret) {
			pr_err("hashing failed ret=%d\n", ret);
			break;
		}
	}

	kfree(output);

out_nomem:
	ahash_request_free(req);

out:
	crypto_free_ahash(tfm);
}
