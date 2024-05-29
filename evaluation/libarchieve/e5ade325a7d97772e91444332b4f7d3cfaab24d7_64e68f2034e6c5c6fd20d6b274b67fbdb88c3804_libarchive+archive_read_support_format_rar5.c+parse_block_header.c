static int parse_block_header(struct archive_read* a, const uint8_t* p,
        ssize_t* block_size, struct compressed_block_header* hdr)
{
    memcpy(hdr, p, sizeof(struct compressed_block_header));

    if(bf_byte_count(hdr) > 2) {
        archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                "Unsupported block header size (was %d, max is 2)",
                bf_byte_count(hdr));
        return ARCHIVE_FATAL;
    }

    /* This should probably use bit reader interface in order to be more
     * future-proof. */
    *block_size = 0;
    switch(bf_byte_count(hdr)) {
        /* 1-byte block size */
        case 0:
            *block_size = *(const uint8_t*) &p[2];
            break;

        /* 2-byte block size */
        case 1:
            *block_size = archive_le16dec(&p[2]);
            break;

        /* 3-byte block size */
        case 2:
            *block_size = archive_le32dec(&p[2]);
            *block_size &= 0x00FFFFFF;
            break;

        /* Other block sizes are not supported. This case is not reached,
         * because we have an 'if' guard before the switch that makes sure
         * of it. */
        default:
            return ARCHIVE_FATAL;
    }

    /* Verify the block header checksum. 0x5A is a magic value and is always
     * constant. */
    uint8_t calculated_cksum = 0x5A
                               ^ (uint8_t) hdr->block_flags_u8
                               ^ (uint8_t) *block_size
                               ^ (uint8_t) (*block_size >> 8)
                               ^ (uint8_t) (*block_size >> 16);

    if(calculated_cksum != hdr->block_cksum) {
        archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                "Block checksum error: got 0x%x, expected 0x%x",
                hdr->block_cksum, calculated_cksum);

        return ARCHIVE_FATAL;
    }

    return ARCHIVE_OK;
}
