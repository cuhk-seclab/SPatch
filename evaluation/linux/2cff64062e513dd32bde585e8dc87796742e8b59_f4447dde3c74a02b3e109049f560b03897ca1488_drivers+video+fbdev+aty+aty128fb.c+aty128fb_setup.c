static int aty128fb_setup(char *options)
{
	char *this_opt;

	if (!options || !*options)
		return 0;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!strncmp(this_opt, "lcd:", 4)) {
			default_lcd_on = simple_strtoul(this_opt+4, NULL, 0);
			continue;
		} else if (!strncmp(this_opt, "crt:", 4)) {
			default_crt_on = simple_strtoul(this_opt+4, NULL, 0);
			continue;
		} else if (!strncmp(this_opt, "backlight:", 10)) {
#ifdef CONFIG_FB_ATY128_BACKLIGHT
			backlight = simple_strtoul(this_opt+10, NULL, 0);
#endif
			continue;
		}
		if(!strncmp(this_opt, "nomtrr", 6)) {
			mtrr = 0;
			continue;
		}
#ifdef CONFIG_PPC_PMAC
		/* vmode and cmode deprecated */
		if (!strncmp(this_opt, "vmode:", 6)) {
			unsigned int vmode = simple_strtoul(this_opt+6, NULL, 0);
			if (vmode > 0 && vmode <= VMODE_MAX)
				default_vmode = vmode;
			continue;
		} else if (!strncmp(this_opt, "cmode:", 6)) {
			unsigned int cmode = simple_strtoul(this_opt+6, NULL, 0);
			switch (cmode) {
			case 0:
			case 8:
				default_cmode = CMODE_8;
				break;
			case 15:
			case 16:
				default_cmode = CMODE_16;
				break;
			case 24:
			case 32:
				default_cmode = CMODE_32;
				break;
			}
			continue;
		}
#endif /* CONFIG_PPC_PMAC */
		mode_option = this_opt;
	}
	return 0;
}
