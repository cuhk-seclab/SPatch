static void speakup_stop_ttys(void)
{
	int i;

	for (i = 0; i < MAX_NR_CONSOLES; i++)
		if ((vc_cons[i].d != NULL) && (vc_cons[i].d->port.tty != NULL))
			stop_tty(vc_cons[i].d->port.tty);
}
