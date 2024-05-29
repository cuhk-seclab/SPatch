time_t
__archive_get_date(time_t now, const char *p)
{
	struct token	tokens[256];
	struct gdstate	_gds;
	struct token	*lasttoken;
	struct gdstate	*gds;
	struct tm	local, *tm;
	struct tm	gmt, *gmt_ptr;
	time_t		Start;
	time_t		tod;
	long		tzone;
#if defined(HAVE__LOCALTIME64_S) || defined(HAVE__GMTIME64_S)
	errno_t		terr;
	__time64_t	tmptime;
#endif

	/* Clear out the parsed token array. */
	memset(tokens, 0, sizeof(tokens));
	/* Initialize the parser state. */
	memset(&_gds, 0, sizeof(_gds));
	gds = &_gds;

	/* Look up the current time. */
#if defined(HAVE_LOCALTIME_R)
	tm = localtime_r(&now, &local);
#elif defined(HAVE__LOCALTIME64_S)
	tmptime = now;
	terr = _localtime64_s(&local, &tmptime);
	if (terr)
		tm = NULL;
	else
		tm = &local;
#else
	memset(&local, 0, sizeof(local));
	tm = localtime(&now);
#endif
	if (tm == NULL)
		return -1;
#if !defined(HAVE_LOCALTIME_R) && !defined(HAVE__LOCALTIME64_S)
	local = *tm;
#endif

	/* Look up UTC if we can and use that to determine the current
	 * timezone offset. */
#if defined(HAVE_GMTIME_R)
	gmt_ptr = gmtime_r(&now, &gmt);
#elif defined(HAVE__GMTIME64_S)
	tmptime = now;
	terr = _gmtime64_s(&gmt, &tmptime);
	if (terr)
		gmt_ptr = NULL;
	else
		gmt_ptr = &gmt;
#else
	memset(&gmt, 0, sizeof(gmt));
	gmt_ptr = gmtime(&now);
	if (gmt_ptr != NULL) {
		/* Copy, in case localtime and gmtime use the same buffer. */
		gmt = *gmt_ptr;
	}
#endif
	if (gmt_ptr != NULL)
		tzone = difftm (&gmt, &local);
	else
		/* This system doesn't understand timezones; fake it. */
		tzone = 0;
	if(local.tm_isdst)
		tzone += HOUR;

	/* Tokenize the input string. */
	lasttoken = tokens;
	while ((lasttoken->token = nexttoken(&p, &lasttoken->value)) != 0) {
		++lasttoken;
		if (lasttoken > tokens + 255)
			return -1;
	}
	gds->tokenp = tokens;

	/* Match phrases until we run out of input tokens. */
	while (gds->tokenp < lasttoken) {
		if (!phrase(gds))
			return -1;
	}

	/* Use current local timezone if none was specified. */
	if (!gds->HaveZone) {
		gds->Timezone = tzone;
		gds->DSTmode = DSTmaybe;
	}

	/* If a timezone was specified, use that for generating the default
	 * time components instead of the local timezone. */
	if (gds->HaveZone && gmt_ptr != NULL) {
		now -= gds->Timezone;
#if defined(HAVE_GMTIME_R)
		gmt_ptr = gmtime_r(&now, &gmt);
#elif defined(HAVE__GMTIME64_S)
		tmptime = now;
		terr = _gmtime64_s(&gmt, &tmptime);
		if (terr)
			gmt_ptr = NULL;
		else
			gmt_ptr = &gmt;
#else
		gmt_ptr = gmtime(&now);
#endif
		if (gmt_ptr != NULL)
			local = *gmt_ptr;
		now += gds->Timezone;
	}

	if (!gds->HaveYear)
		gds->Year = local.tm_year + 1900;
	if (!gds->HaveMonth)
		gds->Month = local.tm_mon + 1;
	if (!gds->HaveDay)
		gds->Day = local.tm_mday;
	/* Note: No default for hour/min/sec; a specifier that just
	 * gives date always refers to 00:00 on that date. */

	/* If we saw more than one time, timezone, weekday, year, month,
	 * or day, then give up. */
	if (gds->HaveTime > 1 || gds->HaveZone > 1 || gds->HaveWeekDay > 1
	    || gds->HaveYear > 1 || gds->HaveMonth > 1 || gds->HaveDay > 1)
		return -1;

	/* Compute an absolute time based on whatever absolute information
	 * we collected. */
	if (gds->HaveYear || gds->HaveMonth || gds->HaveDay
	    || gds->HaveTime || gds->HaveWeekDay) {
		Start = Convert(gds->Month, gds->Day, gds->Year,
		    gds->Hour, gds->Minutes, gds->Seconds,
		    gds->Timezone, gds->DSTmode);
		if (Start < 0)
			return -1;
	} else {
		Start = now;
		if (!gds->HaveRel)
			Start -= local.tm_hour * HOUR + local.tm_min * MINUTE
			    + local.tm_sec;
	}

	/* Add the relative offset. */
	Start += gds->RelSeconds;
	Start += RelativeMonth(Start, gds->Timezone, gds->RelMonth);

	/* Adjust for day-of-week offsets. */
	if (gds->HaveWeekDay
	    && !(gds->HaveYear || gds->HaveMonth || gds->HaveDay)) {
		tod = RelativeDate(Start, gds->Timezone,
		    gds->DSTmode, gds->DayOrdinal, gds->DayNumber);
		Start += tod;
	}

	/* -1 is an error indicator, so return 0 instead of -1 if
	 * that's the actual time. */
	return Start == -1 ? 0 : Start;
}
