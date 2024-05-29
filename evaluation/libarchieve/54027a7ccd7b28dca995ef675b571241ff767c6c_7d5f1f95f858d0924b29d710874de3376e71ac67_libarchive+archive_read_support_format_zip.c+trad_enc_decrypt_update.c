static void
trad_enc_decrypt_update(struct trad_enc_ctx *ctx, const uint8_t *in,
    size_t in_len, uint8_t *out, size_t out_len)
{
	unsigned i, max;

	max = (unsigned)((in_len < out_len)? in_len: out_len);

	for (i = 0; i < max; i++) {
		uint8_t t = in[i] ^ trad_enc_decypt_byte(ctx);
		out[i] = t;
		trad_enc_update_keys(ctx, t);
	}
}
