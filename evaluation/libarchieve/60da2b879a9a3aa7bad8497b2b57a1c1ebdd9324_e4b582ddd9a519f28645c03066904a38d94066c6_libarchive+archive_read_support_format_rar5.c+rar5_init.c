static int rar5_init(struct rar5* rar) {
	memset(rar, 0, sizeof(struct rar5));

	if(CDE_OK != cdeque_init(&rar->cstate.filters, 8192))
		return ARCHIVE_FATAL;

	return ARCHIVE_OK;
}
