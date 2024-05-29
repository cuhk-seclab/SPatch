static int
validate_number_field(const char* p_field, size_t i_size)
{
	unsigned char marker = (unsigned char)p_field[0];
	if (marker == 128 || marker == 255 || marker == 0) {
		/* Base-256 marker, there's nothing we can check. */
		return 1;
	} else {
		/* Must be octal */
		size_t i = 0;
		/* Skip any leading spaces */
		while (i < i_size && p_field[i] == ' ') {
			++i;
		}
		/* Skip octal digits. */
		while (i < i_size && p_field[i] >= '0' && p_field[i] <= '7') {
			++i;
		}
		/* Any remaining characters must be space or NUL padding. */
		while (i < i_size) {
			if (p_field[i] != ' ' && p_field[i] != 0) {
				return 0;
			}
			++i;
		}
		return 1;
	}
}
