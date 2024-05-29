static int
validate_number_field(const char* p_field, size_t i_size)
{
	unsigned char marker = (unsigned char)p_field[0];
	/* octal? */
	if ((marker >= '0' && marker <= '7') || marker == ' ') {
		size_t i = 0;
		for (i = 0; i < i_size; ++i) {
			switch (p_field[i])
			{
			case ' ': /* skip any leading spaces and trailing space*/
				if (octal_found == 0 || i == i_size - 1) {
					continue;
				}
				break;
			case '\0': /* null is allowed only at the end */
				if (i != i_size - 1) {
					return 0;
				}
				break;
			/* rest must be octal digits */
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				++octal_found;
		for (i = 1; i < i_size - 1; ++i) {
				return 0;
			}
		}
		return 1;
	}
	/* base 256 (i.e. binary number) */
	else if (marker == 128 || marker == 255 || marker == 0) {
		/* nothing to check */
		return 1;
	}
	/* not a number field */
	else {
		return 0;
	}
}
