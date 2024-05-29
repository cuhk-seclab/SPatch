static int parse_file_extra_htime(struct archive_read* a,
    struct archive_entry* e, struct rar5* rar, ssize_t* extra_data_size)
{
	char unix_time = 0;
	size_t flags = 0;
	size_t value_len;

	enum HTIME_FLAGS {
		IS_UNIX       = 0x01,
		HAS_MTIME     = 0x02,
		HAS_CTIME     = 0x04,
		HAS_ATIME     = 0x08,
		HAS_UNIX_NS   = 0x10,
	};

	if(!read_var_sized(a, &flags, &value_len))
		return ARCHIVE_EOF;

	*extra_data_size -= value_len;
	if(ARCHIVE_OK != consume(a, value_len)) {
		return ARCHIVE_EOF;
	}

	unix_time = flags & IS_UNIX;

	if(flags & HAS_MTIME) {
		parse_htime_item(a, unix_time, &rar->file.e_mtime,
		    extra_data_size);
		archive_entry_set_mtime(e, rar->file.e_mtime, 0);
	}

	if(flags & HAS_CTIME) {
		parse_htime_item(a, unix_time, &rar->file.e_ctime,
		    extra_data_size);
		archive_entry_set_ctime(e, rar->file.e_ctime, 0);
	}

	if(flags & HAS_ATIME) {
		parse_htime_item(a, unix_time, &rar->file.e_atime,
		    extra_data_size);
		archive_entry_set_atime(e, rar->file.e_atime, 0);
	}

	if(flags & HAS_UNIX_NS) {
		if(!read_u32(a, &rar->file.e_unix_ns))
			return ARCHIVE_EOF;

		*extra_data_size -= 4;
	}

	return ARCHIVE_OK;
}
