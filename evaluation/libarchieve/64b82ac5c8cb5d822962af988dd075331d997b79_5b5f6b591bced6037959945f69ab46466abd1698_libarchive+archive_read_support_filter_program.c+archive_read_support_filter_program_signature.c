int
archive_read_support_filter_program_signature(struct archive *_a,
    const char *cmd, const void *signature, size_t signature_len)
{
	struct archive_read *a = (struct archive_read *)_a;
	struct program_bidder *state;

	/*
	 * Allocate our private state.
	 */
	state = (struct program_bidder *)calloc(1, sizeof (*state));
	if (state == NULL)
		goto memerr;
	state->cmd = strdup(cmd);
	if (state->cmd == NULL)
		goto memerr;

	if (signature != NULL && signature_len > 0) {
		state->signature_len = signature_len;
		state->signature = malloc(signature_len);
		memcpy(state->signature, signature, signature_len);
	}

	if (__archive_read_register_bidder(a, state, NULL,
				&program_bidder_vtable) != ARCHIVE_OK) {
		free_state(state);
		return (ARCHIVE_FATAL);
	}
	return (ARCHIVE_OK);

memerr:
	free_state(state);
	archive_set_error(_a, ENOMEM, "Can't allocate memory");
	return (ARCHIVE_FATAL);
}
