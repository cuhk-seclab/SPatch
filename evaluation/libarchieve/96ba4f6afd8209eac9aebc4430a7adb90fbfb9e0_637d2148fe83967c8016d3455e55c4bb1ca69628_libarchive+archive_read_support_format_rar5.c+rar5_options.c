static int rar5_options(struct archive_read *a, const char *key, const char *val) {
    (void) a;
    (void) key;
    (void) val;

    /* No options supported in this version. Return the ARCHIVE_WARN code to
     * signal the options supervisor that the unpacker didn't handle setting
     * this option. */

    return ARCHIVE_WARN;
}
