static void
xstrftime(struct archive_string *as, const char *fmt, time_t t)
{
/** like strftime(3) but for time_t objects */
	struct tm *rt;
#if defined(HAVE_GMTIME_R) || defined(HAVE_GMTIME_S)
	struct tm timeHere;
#endif
	char strtime[100];
	size_t len;

#if defined(HAVE_GMTIME_S)
	rt = gmtime_s(&timeHere, &t) ? NULL : &timeHere;
#elif defined(HAVE_GMTIME_R)
	rt = gmtime_r(&t, &timeHere);
#else
	rt = gmtime(&t);
#endif
	if (!rt)
		return;
	/* leave the hard yacker to our role model strftime() */
	len = strftime(strtime, sizeof(strtime)-1, fmt, rt);
	archive_strncat(as, strtime, len);
}
