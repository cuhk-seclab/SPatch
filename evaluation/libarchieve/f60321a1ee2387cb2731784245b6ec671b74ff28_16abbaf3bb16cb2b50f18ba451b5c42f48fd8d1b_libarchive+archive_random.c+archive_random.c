int
archive_random(void *buf, size_t nbytes)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
	HCRYPTPROV hProv;
	BOOL success;

	success = CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL,
	    CRYPT_VERIFYCONTEXT);
	if (!success && GetLastError() == (DWORD)NTE_BAD_KEYSET) {
		success = CryptAcquireContext(&hProv, NULL, NULL,
		    PROV_RSA_FULL, CRYPT_NEWKEYSET);
	}
	if (success) {
		success = CryptGenRandom(hProv, (DWORD)nbytes, (BYTE*)buf);
		CryptReleaseContext(hProv, 0);
		if (success)
			return ARCHIVE_OK;
	}
	/* TODO: Does this case really happen? */
	return ARCHIVE_FAILED;
#else
	arc4random_buf(buf, nbytes);
	return ARCHIVE_OK;
#endif
}
