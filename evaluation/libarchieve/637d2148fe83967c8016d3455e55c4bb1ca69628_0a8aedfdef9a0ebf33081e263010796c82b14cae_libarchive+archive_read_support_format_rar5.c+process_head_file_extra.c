static int process_head_file_extra(struct archive_read* a,
        struct archive_entry* e, struct rar5* rar,
        ssize_t extra_data_size)
{
    size_t extra_field_size;
    size_t extra_field_id;
    int ret = ARCHIVE_FATAL;
    size_t var_size;

    enum EXTRA {
        CRYPT = 0x01, HASH = 0x02, HTIME = 0x03, VERSION_ = 0x04,
        REDIR = 0x05, UOWNER = 0x06, SUBDATA = 0x07
    };

    while(extra_data_size > 0) {
        if(!read_var_sized(a, &extra_field_size, &var_size))
            return ARCHIVE_EOF;

        extra_data_size -= var_size;
        if(ARCHIVE_OK != consume(a, var_size)) {
            return ARCHIVE_EOF;
        }

        if(!read_var_sized(a, &extra_field_id, &var_size))
            return ARCHIVE_EOF;

        extra_data_size -= var_size;
        if(ARCHIVE_OK != consume(a, var_size)) {
            return ARCHIVE_EOF;
        }

        switch(extra_field_id) {
            case HASH:
                ret = parse_file_extra_hash(a, rar, &extra_data_size);
                break;
            case HTIME:
                ret = parse_file_extra_htime(a, e, rar, &extra_data_size);
                break;
            case CRYPT:
                fallthrough;
            case VERSION_:
                fallthrough;
            case REDIR:
                fallthrough;
            case UOWNER:
                fallthrough;
            case SUBDATA:
                fallthrough;
            default:
                archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                        "Unknown extra field in file/service block: 0x%02x",
                        (int) extra_field_id);
                return ARCHIVE_FATAL;
        }
    }

    if(ret != ARCHIVE_OK) {
        /* Attribute not implemented. */
        return ret;
    }

    return ARCHIVE_OK;
}
