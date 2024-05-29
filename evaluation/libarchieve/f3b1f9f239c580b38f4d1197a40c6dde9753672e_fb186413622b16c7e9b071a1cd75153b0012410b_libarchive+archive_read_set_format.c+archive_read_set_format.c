int
archive_read_set_format(struct archive *_a, int code)
{
  int r1, r2, slots, i;
  char str[10];
  struct archive_read *a = (struct archive_read *)_a;

  if ((r1 = archive_read_support_format_by_code(_a, code)) < (ARCHIVE_OK))
    return r1;

  r1 = r2 = (ARCHIVE_OK);
  if (a->format)
    r2 = (ARCHIVE_WARN);
  switch (code & ARCHIVE_FORMAT_BASE_MASK)
  {
    case ARCHIVE_FORMAT_7ZIP:
      strcpy(str, "7zip");
      break;
    case ARCHIVE_FORMAT_AR:
      strcpy(str, "ar");
      break;
    case ARCHIVE_FORMAT_CAB:
      strcpy(str, "cab");
      break;
    case ARCHIVE_FORMAT_CPIO:
      strcpy(str, "cpio");
      break;
    case ARCHIVE_FORMAT_EMPTY:
      strcpy(str, "empty");
      break;
    case ARCHIVE_FORMAT_ISO9660:
      strcpy(str, "iso9660");
      break;
    case ARCHIVE_FORMAT_LHA:
      strcpy(str, "lha");
      break;
    case ARCHIVE_FORMAT_MTREE:
      strcpy(str, "mtree");
      break;
    case ARCHIVE_FORMAT_RAR:
      strcpy(str, "rar");
      break;
    case ARCHIVE_FORMAT_RAR_V5:
      strcpy(str, "rar5");
      break;
    case ARCHIVE_FORMAT_RAW:
      strcpy(str, "raw");
      break;
    case ARCHIVE_FORMAT_TAR:
      strcpy(str, "tar");
      break;
    case ARCHIVE_FORMAT_WARC:
      strcpy(str, "warc");
      break;
    case ARCHIVE_FORMAT_XAR:
      strcpy(str, "xar");
      break;
    case ARCHIVE_FORMAT_ZIP:
      strcpy(str, "zip");
      break;
    default:
      archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
          "Invalid format code specified");
      return (ARCHIVE_FATAL);
  }

  slots = sizeof(a->formats) / sizeof(a->formats[0]);
  a->format = &(a->formats[0]);
  for (i = 0; i < slots; i++, a->format++) {
    if (!a->format->name || !strcmp(a->format->name, str))
      break;
  }
  if (!a->format->name || strcmp(a->format->name, str))
  {
    archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
        "Internal error: Unable to set format");
    r1 = (ARCHIVE_FATAL);
  }

  return (r1 < r2) ? r1 : r2;
}
