static int do_uncompress_block(struct archive_read* a, const uint8_t* p) {
    struct rar5* rar = get_context(a);
    uint16_t num;
    int ret;

    const uint64_t cmask = rar->cstate.window_mask;
    const struct compressed_block_header* hdr = &rar->last_block_hdr;
    const uint8_t bit_size = 1 + bf_bit_size(hdr);

    while(1) {
        if(rar->cstate.write_ptr - rar->cstate.last_write_ptr >
                (rar->cstate.window_size >> 1)) {

            /* Don't allow growing data by more than half of the window size
             * at a time. In such case, break the loop; next call to this
             * function will continue processing from this moment. */

            break;
        }

        if(rar->bits.in_addr > rar->cstate.cur_block_size - 1 ||
                (rar->bits.in_addr == rar->cstate.cur_block_size - 1 &&
                 rar->bits.bit_addr >= bit_size))
        {
            /* If the program counter is here, it means the function has
             * finished processing the block. */
            rar->cstate.block_parsing_finished = 1;
            break;
        }

        /* Decode the next literal. */
        if(ARCHIVE_OK != decode_number(a, &rar->cstate.ld, p, &num)) {
            return ARCHIVE_EOF;
        }

        /* Num holds a decompression literal, or 'command code'.
         *
         * - Values lower than 256 are just bytes. Those codes can be stored
         *   in the output buffer directly.
         *
         * - Code 256 defines a new filter, which is later used to transform
         *   the data block accordingly to the filter type. The data block
         *   needs to be fully uncompressed first.
         *
         * - Code bigger than 257 and smaller than 262 define a repetition
         *   pattern that should be copied from an already uncompressed chunk
         *   of data.
         */

        if(num < 256) {
            /* Directly store the byte. */

            int64_t write_idx = rar->cstate.solid_offset +
                rar->cstate.write_ptr++;

            rar->cstate.window_buf[write_idx & cmask] = (uint8_t) num;
            continue;
        } else if(num >= 262) {
            uint16_t dist_slot;
            int len = decode_code_length(rar, p, num - 262),
                dbits,
                dist = 1;

            if(len == -1) {
                archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
                    "Failed to decode the code length");

                return ARCHIVE_FATAL;
            }

            if(ARCHIVE_OK != decode_number(a, &rar->cstate.dd, p, &dist_slot))
            {
                archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
                    "Failed to decode the distance slot");

                return ARCHIVE_FATAL;
            }

            if(dist_slot < 4) {
                dbits = 0;
                dist += dist_slot;
            } else {
                dbits = dist_slot / 2 - 1;

                /* Cast to uint32_t will make sure the shift left operation
                 * won't produce undefined result. Then, the uint32_t type will
                 * be implicitly casted to int. */
                dist += (uint32_t) (2 | (dist_slot & 1)) << dbits;
            }

            if(dbits > 0) {
                if(dbits >= 4) {
                    uint32_t add = 0;
                    uint16_t low_dist;

                    if(dbits > 4) {
                        if(ARCHIVE_OK != read_bits_32(rar, p, &add)) {
                            /* Return EOF if we can't read more data. */
                            return ARCHIVE_EOF;
                        }

                        skip_bits(rar, dbits - 4);
                        add = (add >> (36 - dbits)) << 4;
                        dist += add;
                    }

                    if(ARCHIVE_OK != decode_number(a, &rar->cstate.ldd, p,
                                &low_dist))
                    {
                        archive_set_error(&a->archive,
                                ARCHIVE_ERRNO_PROGRAMMER,
                                "Failed to decode the distance slot");

                        return ARCHIVE_FATAL;
                    }

                        /* This only happens in invalid archives. */
                        archive_set_error(&a->archive,
                            ARCHIVE_ERRNO_FILE_FORMAT,
                            "Distance pointer overflow");
                        return ARCHIVE_FATAL;
                    }
                    dist += low_dist;
                } else {
                    /* dbits is one of [0,1,2,3] */
                    int add;

                    if(ARCHIVE_OK != read_consume_bits(rar, p, dbits, &add)) {
                        /* Return EOF if we can't read more data. */
                        return ARCHIVE_EOF;
                    }

                    dist += add;
                }
            }

            if(dist > 0x100) {
                len++;

                if(dist > 0x2000) {
                    len++;

                    if(dist > 0x40000) {
                        len++;
                    }
                }
            }

            dist_cache_push(rar, dist);
            rar->cstate.last_len = len;

            if(ARCHIVE_OK != copy_string(a, len, dist))
                return ARCHIVE_FATAL;

            continue;
        } else if(num == 256) {
            /* Create a filter. */
            ret = parse_filter(a, p);
            if(ret != ARCHIVE_OK)
                return ret;

            continue;
        } else if(num == 257) {
            if(rar->cstate.last_len != 0) {
                if(ARCHIVE_OK != copy_string(a, rar->cstate.last_len,
                            rar->cstate.dist_cache[0]))
                {
                    return ARCHIVE_FATAL;
                }
            }

            continue;
        } else if(num < 262) {
            const int idx = num - 258;
            const int dist = dist_cache_touch(rar, idx);

            uint16_t len_slot;
            int len;

            if(ARCHIVE_OK != decode_number(a, &rar->cstate.rd, p, &len_slot)) {
                return ARCHIVE_FATAL;
            }

            len = decode_code_length(rar, p, len_slot);
            rar->cstate.last_len = len;

            if(ARCHIVE_OK != copy_string(a, len, dist))
                return ARCHIVE_FATAL;

            continue;
        }

        /* The program counter shouldn't reach here. */
        archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                "Unsupported block code: 0x%x", num);

        return ARCHIVE_FATAL;
    }

    return ARCHIVE_OK;
}
