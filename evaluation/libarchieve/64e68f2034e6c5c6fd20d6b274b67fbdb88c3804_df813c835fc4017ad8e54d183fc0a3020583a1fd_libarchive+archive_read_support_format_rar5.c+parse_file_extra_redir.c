static int parse_file_extra_redir(struct archive_read* a,
        struct archive_entry* e, struct rar5* rar,
        ssize_t* extra_data_size)
{
	size_t value_len = 0;
	char target_utf8_buf[2048 * 4];
	const uint8_t* p;

	if(!read_var_sized(a, &rar->file.redir_type, &value_len))
		return ARCHIVE_EOF;
	if(ARCHIVE_OK != consume(a, value_len))
		return ARCHIVE_EOF;
	*extra_data_size -= value_len;

	if(!read_var_sized(a, &rar->file.redir_flags, &value_len))
		return ARCHIVE_EOF;
	if(ARCHIVE_OK != consume(a, value_len))
		return ARCHIVE_EOF;
	*extra_data_size -= value_len;

	if(!read_var_sized(a, &value_len, NULL))
		return ARCHIVE_EOF;
        *extra_data_size -= value_len + 1;
	if(!read_ahead(a, value_len, &p))
		return ARCHIVE_EOF;

	if(value_len > 2047) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Link target is too long");
		return ARCHIVE_FATAL;
	}
	if(value_len == 0) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "No link target specified");
		return ARCHIVE_FATAL;
	}
	memcpy(target_utf8_buf, p, value_len);
	target_utf8_buf[value_len] = 0;

	if(ARCHIVE_OK != consume(a, value_len))
		return ARCHIVE_EOF;

	switch(rar->file.redir_type) {
		case REDIR_TYPE_UNIXSYMLINK:
		case REDIR_TYPE_WINSYMLINK:
			archive_entry_set_filetype(e, AE_IFLNK);
			archive_entry_update_symlink_utf8(e, target_utf8_buf);
			if (rar->file.redir_flags & REDIR_SYMLINK_IS_DIR) {
				archive_entry_set_symlink_type(e,
				    AE_SYMLINK_TYPE_DIRECTORY);
			} else {
				archive_entry_set_symlink_type(e,
			    AE_SYMLINK_TYPE_FILE);
			}
			break;

		case REDIR_TYPE_HARDLINK:
			archive_entry_set_filetype(e, AE_IFREG);
			archive_entry_update_hardlink_utf8(e, target_utf8_buf);
			break;

		default:
			/* Unknown redir type */
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Unsupported redir type: %d",
			    (int)rar->file.redir_type);
			return ARCHIVE_FATAL;
			break;
	}
	return ARCHIVE_OK;
}
