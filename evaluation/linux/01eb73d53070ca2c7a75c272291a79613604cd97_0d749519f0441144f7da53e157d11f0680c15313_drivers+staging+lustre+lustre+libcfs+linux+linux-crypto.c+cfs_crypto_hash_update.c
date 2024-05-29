int cfs_crypto_hash_update(struct cfs_crypto_hash_desc *hdesc,
			   const void *buf, unsigned int buf_len)
{
	struct scatterlist sl;

	sg_init_one(&sl, buf, buf_len);

	return crypto_hash_update((struct hash_desc *)hdesc, &sl, sl.length);
}
