const char *
archive_liblz4_version(void)
{
#if defined(HAVE_LZ4_H) && defined(HAVE_LIBLZ4)
#define str(s) #s
#define NUMBER(x) str(x)
	return NUMBER(LZ4_VERSION_MAJOR) "." NUMBER(LZ4_VERSION_MINOR) "." NUMBER(LZ4_VERSION_RELEASE);
#undef NUMBER
#undef str
#else
	return NULL;
#endif
}
