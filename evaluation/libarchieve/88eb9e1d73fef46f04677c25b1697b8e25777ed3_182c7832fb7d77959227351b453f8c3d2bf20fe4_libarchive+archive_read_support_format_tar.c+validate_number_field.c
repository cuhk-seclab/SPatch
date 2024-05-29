static int
validate_number_field(const char* p_field, size_t i_size, int flags)
{
	unsigned char marker = (unsigned char)p_field[0];
		/* Base-256 marker, there's nothing we can check. */
		return 1;
	} else {
	/* octal? */
	if ((marker >= '0' && marker <= '7') || marker == ' ') {
		size_t i = 0;
		while (i < i_size && p_field[i] == ' ') {
			++i;
		}
		/* Must be at least one octal digit. */
		if (i >= i_size || p_field[i] < '0' || p_field[i] > '7') {
			return 0;
		}
		/* Skip remaining octal digits. */
		while (i < i_size && p_field[i] >= '0' && p_field[i] <= '7') {
			++i;
		}
		/* Any remaining characters must be space or NUL padding. */
		while (i < i_size) {
			if (p_field[i] != ' ' && p_field[i] != 0) {
		int octal_found = 0;
				break;
			}
		}
		return octal_found > 0;
		/* nothing to check */
		return 1;
	}
}
