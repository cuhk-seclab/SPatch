int
__archive_read_program(struct archive_read_filter *self, const char *cmd)
{
	struct program_filter	*state;
	static const size_t out_buf_len = 65536;
	char *out_buf;
	const char *prefix = "Program: ";
	pid_t child;
	size_t l;

	l = strlen(prefix) + strlen(cmd) + 1;
	state = (struct program_filter *)calloc(1, sizeof(*state));
	out_buf = (char *)malloc(out_buf_len);
	if (state == NULL || out_buf == NULL ||
	    archive_string_ensure(&state->description, l) == NULL) {
		archive_set_error(&self->archive->archive, ENOMEM,
		    "Can't allocate input data");
		if (state != NULL) {
			archive_string_free(&state->description);
			free(state);
		}
		free(out_buf);
		return (ARCHIVE_FATAL);
	}
	archive_strcpy(&state->description, prefix);
	archive_strcat(&state->description, cmd);

	self->code = ARCHIVE_FILTER_PROGRAM;
	self->name = state->description.s;

	state->out_buf = out_buf;
	state->out_buf_len = out_buf_len;

	child = __archive_create_child(cmd, &state->child_stdin,
	    &state->child_stdout);
	if (child == -1) {
		free(state->out_buf);
		archive_string_free(&state->description);
		free(state);
		archive_set_error(&self->archive->archive, EINVAL,
		    "Can't initialize filter; unable to run program \"%s\"",
		    cmd);
		return (ARCHIVE_FATAL);
	}
#if defined(_WIN32) && !defined(__CYGWIN__)
	state->child = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, child);
	if (state->child == NULL) {
		child_stop(self, state);
		free(state->out_buf);
		archive_string_free(&state->description);
		free(state);
		archive_set_error(&self->archive->archive, EINVAL,
		    "Can't initialize filter; unable to run program \"%s\"",
		    cmd);
		return (ARCHIVE_FATAL);
	}
#else
	state->child = child;
#endif

	self->data = state;
	self->read = program_filter_read;
	self->skip = NULL;
	self->close = program_filter_close;

	/* XXX Check that we can read at least one byte? */
	return (ARCHIVE_OK);
}
