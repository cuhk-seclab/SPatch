int
archive_read_support_format_rar(struct archive *_a)
{
  struct archive_read *a = (struct archive_read *)_a;
  struct rar *rar;
  int r;

  archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW,
                      "archive_read_support_format_rar");

  rar = (struct rar *)calloc(sizeof(*rar), 1);
  if (rar == NULL)
  {
    archive_set_error(&a->archive, ENOMEM, "Can't allocate rar data");
    return (ARCHIVE_FATAL);
  }

	/*
	 * Until enough data has been read, we cannot tell about
	 * any encrypted entries yet.
	 */
	rar->has_encrypted_entries = ARCHIVE_READ_FORMAT_ENCRYPTION_DONT_KNOW;

  r = __archive_read_register_format(a,
                                     rar,
                                     "rar",
                                     archive_read_format_rar_bid,
                                     archive_read_format_rar_options,
                                     archive_read_format_rar_read_header,
                                     archive_read_format_rar_read_data,
                                     archive_read_format_rar_read_data_skip,
                                     archive_read_format_rar_seek_data,
                                     archive_read_format_rar_cleanup,
                                     archive_read_support_format_rar_capabilities,
                                     archive_read_format_rar_has_encrypted_entries);

  if (r != ARCHIVE_OK)
    free(rar);
  return (r);
}
