static int wtm_init(struct snd_ice1712 *ice)
{
	static unsigned short stac_inits_wtm[] = {
		STAC946X_RESET, 0,
		(unsigned short)-1
	};
	unsigned short *p;

	/*WTM 192M*/
	ice->num_total_dacs = 8;
	ice->num_total_adcs = 4;
	ice->force_rdma1 = 1;

	/*initialize codec*/
	p = stac_inits_wtm;
	for (; *p != (unsigned short)-1; p += 2) {
		stac9460_put(ice, p[0], p[1]);
		stac9460_2_put(ice, p[0], p[1]);
	}
	return 0;
}
