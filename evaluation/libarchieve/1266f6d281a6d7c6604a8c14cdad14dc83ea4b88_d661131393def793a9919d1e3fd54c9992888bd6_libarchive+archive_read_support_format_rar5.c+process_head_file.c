static int process_head_file(struct archive_read* a, struct rar5* rar,
        struct archive_entry* entry, size_t block_flags)
{
    ssize_t extra_data_size = 0;
    size_t data_size = 0;
    size_t file_flags = 0;
    size_t file_attr = 0;
    size_t compression_info = 0;
    size_t host_os = 0;
    size_t name_size = 0;
    uint64_t unpacked_size;
    uint32_t mtime = 0, crc;
    int c_method = 0, c_version = 0, is_dir;
    char name_utf8_buf[2048 * 4];
    const uint8_t* p;

    memset(entry, 0, sizeof(struct archive_entry));

    /* Do not reset file context if we're switching archives. */
    if(!rar->cstate.switch_multivolume) {
        reset_file_context(rar);
    }

    if(block_flags & HFL_EXTRA_DATA) {
        size_t edata_size = 0;
        if(!read_var_sized(a, &edata_size, NULL))
            return ARCHIVE_EOF;

        /* Intentional type cast from unsigned to signed. */
        extra_data_size = (ssize_t) edata_size;
    }

    if(block_flags & HFL_DATA) {
        if(!read_var_sized(a, &data_size, NULL))
            return ARCHIVE_EOF;

        rar->file.bytes_remaining = data_size;
    } else {
        rar->file.bytes_remaining = 0;

        archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                "no data found in file/service block");
        return ARCHIVE_FATAL;
    }

    enum FILE_FLAGS {
        DIRECTORY = 0x0001, UTIME = 0x0002, CRC32 = 0x0004,
        UNKNOWN_UNPACKED_SIZE = 0x0008,
    };

    enum COMP_INFO_FLAGS {
        SOLID = 0x0040,
    };

    if(!read_var_sized(a, &file_flags, NULL))
        return ARCHIVE_EOF;

    if(!read_var(a, &unpacked_size, NULL))
        return ARCHIVE_EOF;

    if(file_flags & UNKNOWN_UNPACKED_SIZE) {
        archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
                "Files with unknown unpacked size are not supported");
        return ARCHIVE_FATAL;
    }

    is_dir = (int) (file_flags & DIRECTORY);

    if(!read_var_sized(a, &file_attr, NULL))
        return ARCHIVE_EOF;

    if(file_flags & UTIME) {
        if(!read_u32(a, &mtime))
            return ARCHIVE_EOF;
    }

    if(file_flags & CRC32) {
        if(!read_u32(a, &crc))
            return ARCHIVE_EOF;
    }

    if(!read_var_sized(a, &compression_info, NULL))
        return ARCHIVE_EOF;

    c_method = (int) (compression_info >> 7) & 0x7;
    c_version = (int) (compression_info & 0x3f);

    rar->cstate.window_size = is_dir ?
        0 :
        g_unpack_window_size << ((compression_info >> 10) & 15);
    rar->cstate.method = c_method;
    rar->cstate.version = c_version + 50;

    rar->file.solid = (compression_info & SOLID) > 0;
    rar->file.service = 0;

    if(!read_var_sized(a, &host_os, NULL))
        return ARCHIVE_EOF;

    enum HOST_OS {
        HOST_WINDOWS = 0,
        HOST_UNIX = 1,
    };

    if(host_os == HOST_WINDOWS) {
        /* Host OS is Windows */

        unsigned short mode = 0660;

        if(is_dir)
            mode |= AE_IFDIR;
        else
            mode |= AE_IFREG;

        archive_entry_set_mode(entry, mode);
    } else if(host_os == HOST_UNIX) {
        /* Host OS is Unix */
        archive_entry_set_mode(entry, (unsigned short) file_attr);
    } else {
        /* Unknown host OS */
        archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                "Unsupported Host OS: 0x%02x", (int) host_os);

        return ARCHIVE_FATAL;
    }

    if(!read_var_sized(a, &name_size, NULL))
        return ARCHIVE_EOF;

    if(!read_ahead(a, name_size, &p))
        return ARCHIVE_EOF;

    if(name_size > 2047) {
        archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                "Filename is too long");

        return ARCHIVE_FATAL;
    }

    if(name_size == 0) {
        archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                "No filename specified");

        return ARCHIVE_FATAL;
    }

    memcpy(name_utf8_buf, p, name_size);
    name_utf8_buf[name_size] = 0;
    if(ARCHIVE_OK != consume(a, name_size)) {
        return ARCHIVE_EOF;
    }

    if(extra_data_size > 0) {
        int ret = process_head_file_extra(a, entry, rar, extra_data_size);

        /* Sanity check. */
        if(extra_data_size < 0) {
            archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
                    "File extra data size is not zero");
            return ARCHIVE_FATAL;
        }

        if(ret != ARCHIVE_OK)
            return ret;
    }

    if((file_flags & UNKNOWN_UNPACKED_SIZE) == 0) {
        rar->file.unpacked_size = (ssize_t) unpacked_size;
        archive_entry_set_size(entry, unpacked_size);
    }

    if(file_flags & UTIME) {
        archive_entry_set_mtime(entry, (time_t) mtime, 0);
    }

    if(file_flags & CRC32) {
        rar->file.stored_crc32 = crc;
    }

    archive_entry_update_pathname_utf8(entry, name_utf8_buf);

    if(!rar->cstate.switch_multivolume) {
        /* Do not reinitialize unpacking state if we're switching archives. */
        rar->cstate.block_parsing_finished = 1;
        rar->cstate.all_filters_applied = 1;
        rar->cstate.initialized = 0;
    }

    if(rar->generic.split_before > 0) {
        /* If now we're standing on a header that has a 'split before' mark,
         * it means we're standing on a 'continuation' file header. Signal
         * the caller that if it wants to move to another file, it must call
         * rar5_read_header() function again. */

        return ARCHIVE_RETRY;
    } else {
        return ARCHIVE_OK;
    }
}
