int archive_read_support_format_rar5(struct archive *_a) {
	struct archive_read* ar;
	int ret;
	struct rar5* rar;

	if(ARCHIVE_OK != (ret = get_archive_read(_a, &ar)))
		return ret;

	rar = malloc(sizeof(*rar));
	if(rar == NULL) {
		archive_set_error(&ar->archive, ENOMEM,
		    "Can't allocate rar5 data");
		return ARCHIVE_FATAL;
	}

	if(ARCHIVE_OK != rar5_init(rar)) {
		archive_set_error(&ar->archive, ENOMEM,
		    "Can't allocate rar5 filter buffer");
		free(rar);
		return ARCHIVE_FATAL;
	}

	ret = __archive_read_register_format(ar,
	    rar,
	    "rar5",
	    rar5_bid,
	    rar5_options,
	    rar5_read_header,
	    rar5_read_data,
	    rar5_read_data_skip,
	    rar5_seek_data,
	    rar5_cleanup,
	    rar5_capabilities,
	    rar5_has_encrypted_entries);

	if(ret != ARCHIVE_OK) {
		(void) rar5_cleanup(ar);
	}

	return ret;
}
