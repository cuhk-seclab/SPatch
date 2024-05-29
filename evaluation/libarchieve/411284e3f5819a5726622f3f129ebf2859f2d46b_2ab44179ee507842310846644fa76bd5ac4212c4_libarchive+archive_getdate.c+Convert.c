static time_t
Convert(time_t Month, time_t Day, time_t Year,
	time_t Hours, time_t Minutes, time_t Seconds,
	time_t Timezone, enum DSTMODE DSTmode)
{
	signed char DaysInMonth[12] = {
		31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	time_t		Julian;
	int		i;
	struct tm	*ltime;
#if defined(HAVE_LOCALTIME_R) || defined(HAVE__LOCALTIME64_S)
	struct tm	tmbuf;
#endif
#if defined(HAVE__LOCALTIME64_S)
	errno_t		terr;
	__time64_t	tmptime;
#endif

	if (Year < 69)
		Year += 2000;
	else if (Year < 100)
		Year += 1900;
	DaysInMonth[1] = Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0)
	    ? 29 : 28;
	/* Checking for 2038 bogusly assumes that time_t is 32 bits.  But
	   I'm too lazy to try to check for time_t overflow in another way.  */
	if (Year < EPOCH || Year >= 2038
	    || Month < 1 || Month > 12
	    /* Lint fluff:  "conversion from long may lose accuracy" */
	    || Day < 1 || Day > DaysInMonth[(int)--Month]
	    || Hours < 0 || Hours > 23
	    || Minutes < 0 || Minutes > 59
	    || Seconds < 0 || Seconds > 59)
		return -1;

	Julian = Day - 1;
	for (i = 0; i < Month; i++)
		Julian += DaysInMonth[i];
	for (i = EPOCH; i < Year; i++)
		Julian += 365 + (i % 4 == 0);
	Julian *= DAY;
	Julian += Timezone;
	Julian += Hours * HOUR + Minutes * MINUTE + Seconds;
#if defined(HAVE_LOCALTIME_R)
	ltime = localtime_r(&Julian, &tmbuf);
#elif defined(HAVE__LOCALTIME64_S)
	tmptime = Julian;
	terr = _localtime64_s(&tmbuf, &tmptime);
	if (terr)
		ltime = NULL;
	else
		ltime = &tmbuf;
#else
	ltime = localtime(&Julian);
#endif
	if (DSTmode == DSTon
	    || (DSTmode == DSTmaybe && ltime->tm_isdst))
		Julian -= HOUR;
	return Julian;
}
