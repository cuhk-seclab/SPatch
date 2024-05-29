static void
get_tmfromtime(struct tm *tm, time_t *t)
{
#if HAVE_LOCALTIME_R
	tzset();
	localtime_r(t, tm);
#elif HAVE__LOCALTIME64_S
	__time64_t tmp_t = (__time64_t) *t; //time_t may be shorter than 64 bits
	_localtime64_s(tm, &tmp_t);
#else
	memcpy(tm, localtime(t), sizeof(*tm));
#endif
}
