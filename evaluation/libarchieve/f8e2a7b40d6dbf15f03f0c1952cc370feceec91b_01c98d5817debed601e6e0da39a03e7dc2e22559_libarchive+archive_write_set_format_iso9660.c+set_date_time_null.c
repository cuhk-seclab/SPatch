static void
set_date_time_null(unsigned char *p)
{
	memset(p, (int)'0', 16);
	p[16] = 0;
}
