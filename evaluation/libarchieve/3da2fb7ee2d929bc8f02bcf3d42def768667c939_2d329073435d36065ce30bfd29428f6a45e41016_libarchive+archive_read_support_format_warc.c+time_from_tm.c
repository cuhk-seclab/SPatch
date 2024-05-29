static time_t
time_from_tm(struct tm *t)
{
#if HAVE__MKGMTIME64
        return (_mkgmtime64(t));
#elif HAVE_TIMEGM
        /* Use platform timegm() if available. */
        return (timegm(t));
#else
        /* Else use direct calculation using POSIX assumptions. */
        /* First, fix up tm_yday based on the year/month/day. */
        if (mktime(t) == (time_t)-1)
                return ((time_t)-1);
        /* Then we can compute timegm() from first principles. */
        return (t->tm_sec
            + t->tm_min * 60
            + t->tm_hour * 3600
            + t->tm_yday * 86400
            + (t->tm_year - 70) * 31536000
            + ((t->tm_year - 69) / 4) * 86400
            - ((t->tm_year - 1) / 100) * 86400
            + ((t->tm_year + 299) / 400) * 86400);
#endif
}
