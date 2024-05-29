int cmdline_find_option_bool(const char *cmdline, const char *option)
{
	char c;
	int pos = 0, wstart = 0;
	const char *opptr = NULL;
	enum {
		st_wordstart = 0,	/* Start of word/after whitespace */
		st_wordcmp,	/* Comparing this word */
		st_wordskip,	/* Miscompare, skip */
	} state = st_wordstart;

	if (!cmdline)
		return -1;      /* No command line */

	if (!strlen(cmdline))
		return 0;

	/*
	 * This 'pos' check ensures we do not overrun
	 * a non-NULL-terminated 'cmdline'
	 */
	while (pos < COMMAND_LINE_SIZE) {
		c = *(char *)cmdline++;
		pos++;

		switch (state) {
		case st_wordstart:
			if (!c)
				return 0;
			else if (myisspace(c))
				break;

			state = st_wordcmp;
			opptr = option;
			wstart = pos;
			/* fall through */

		case st_wordcmp:
			if (!*opptr) {
				/*
				 * We matched all the way to the end of the
				 * option we were looking for.  If the
				 * command-line has a space _or_ ends, then
				 * we matched!
				 */
				if (!c || myisspace(c))
					return wstart;
				else
					state = st_wordskip;
			} else if (!c) {
				/*
				 * Hit the NULL terminator on the end of
				 * cmdline.
				 */
				return 0;
			} else if (c != *opptr++) {
				state = st_wordskip;
			}
			break;

		case st_wordskip:
			if (!c)
				return 0;
			else if (myisspace(c))
				state = st_wordstart;
			break;
		}
	}

	return 0;	/* Buffer overrun */
}
