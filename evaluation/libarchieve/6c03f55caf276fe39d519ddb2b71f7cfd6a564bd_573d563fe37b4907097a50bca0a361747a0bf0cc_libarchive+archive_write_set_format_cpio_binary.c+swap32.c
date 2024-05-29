static uint32_t swap32(uint32_t in) {
	union {
		uint32_t l;
		uint16_t s[2];
		uint8_t c[4];
	} U;
	U.l = 1;
	if (U.c[0]) {		/* Little-endian */
		uint16_t t;
		U.l = in;
		t = U.s[0];
		U.s[0] = U.s[1];
		U.s[1] = t;
	} else if (U.c[3]) {	/* Big-endian */
		U.l = in;
		U.s[0] = swap16(U.s[0]);
		U.s[1] = swap16(U.s[1]);
	} else {		/* PDP-endian */
		U.l = in;
	}
	return U.l;
}
