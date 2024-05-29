static int
set_bidder_signature(struct archive_read_filter_bidder *bidder,
    struct program_bidder *state, const void *signature, size_t signature_len)
{

	if (signature != NULL && signature_len > 0) {
		state->signature_len = signature_len;
		state->signature = malloc(signature_len);
		memcpy(state->signature, signature, signature_len);
	}

	/*
	 * Fill in the bidder object.
	 */
	bidder->data = state;
	bidder->bid = program_bidder_bid;
	bidder->init = program_bidder_init;
	bidder->free = program_bidder_free;
	return (ARCHIVE_OK);
}
