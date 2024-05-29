static void
get_tmfromtime(struct tm *tm, time_t *t)
{
#if HAVE_LOCALTIME_S
	localtime_s(tm, t);
#elif HAVE_LOCALTIME_R
	tzset();
	localtime_r(t, tm);
#else
	memcpy(tm, localtime(t), sizeof(*tm));
#endif
}
