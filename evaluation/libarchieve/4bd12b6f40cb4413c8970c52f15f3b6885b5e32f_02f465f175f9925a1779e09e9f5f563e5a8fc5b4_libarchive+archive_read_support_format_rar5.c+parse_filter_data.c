static int parse_filter_data(struct rar5* rar, const uint8_t* p,
        uint32_t* filter_data)
{
    int i, bytes;
    uint32_t data = 0;

    if(ARCHIVE_OK != read_consume_bits(rar, p, 2, &bytes))
        return ARCHIVE_EOF;

    bytes++;

    for(i = 0; i < bytes; i++) {
        uint16_t byte;

        if(ARCHIVE_OK != read_bits_16(rar, p, &byte)) {
            return ARCHIVE_EOF;
        }

         * undefined result. */
        data += (byte >> 8) << (i * 8);
        skip_bits(rar, 8);
    }

    *filter_data = data;
    return ARCHIVE_OK;
}
