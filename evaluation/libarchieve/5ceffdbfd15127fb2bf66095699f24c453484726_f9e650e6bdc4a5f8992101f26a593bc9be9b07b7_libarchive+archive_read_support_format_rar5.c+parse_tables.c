static int parse_tables(struct archive_read* a, struct rar5* rar,
    const uint8_t* p)
{
	int ret, value, i, w, idx = 0;
	uint8_t bit_length[HUFF_BC],
		table[HUFF_TABLE_SIZE],
		nibble_mask = 0xF0,
		nibble_shift = 4;

	enum { ESCAPE = 15 };

	/* The data for table generation is compressed using a simple RLE-like
	 * algorithm when storing zeroes, so we need to unpack it first. */
	for(w = 0, i = 0; w < HUFF_BC;) {
		if(i >= rar->cstate.cur_block_size) {
			/* Truncated data, can't continue. */
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Truncated data in huffman tables");
			return ARCHIVE_FATAL;
		}

		value = (p[i] & nibble_mask) >> nibble_shift;

		if(nibble_mask == 0x0F)
			++i;

		nibble_mask ^= 0xFF;
		nibble_shift ^= 4;

		/* Values smaller than 15 is data, so we write it directly.
		 * Value 15 is a flag telling us that we need to unpack more
		 * bytes. */
		if(value == ESCAPE) {
			value = (p[i] & nibble_mask) >> nibble_shift;
			if(nibble_mask == 0x0F)
				++i;
			nibble_mask ^= 0xFF;
			nibble_shift ^= 4;

			if(value == 0) {
				/* We sometimes need to write the actual value
				 * of 15, so this case handles that. */
				bit_length[w++] = ESCAPE;
			} else {
				int k;

				/* Fill zeroes. */
				for(k = 0; (k < value + 2) && (w < HUFF_BC);
				    k++) {
					bit_length[w++] = 0;
				}
			}
		} else {
			bit_length[w++] = value;
		}
	}

	rar->bits.in_addr = i;
	rar->bits.bit_addr = nibble_shift ^ 4;

	ret = create_decode_tables(bit_length, &rar->cstate.bd, HUFF_BC);
	if(ret != ARCHIVE_OK) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Decoding huffman tables failed");
		return ARCHIVE_FATAL;
	}

	for(i = 0; i < HUFF_TABLE_SIZE;) {
		uint16_t num;

		if((rar->bits.in_addr + 6) >= rar->cstate.cur_block_size) {
			/* Truncated data, can't continue. */
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Truncated data in huffman tables (#2)");
			return ARCHIVE_FATAL;
		}

		ret = decode_number(a, &rar->cstate.bd, p, &num);
		if(ret != ARCHIVE_OK) {
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_FILE_FORMAT,
			    "Decoding huffman tables failed");
			return ARCHIVE_FATAL;
		}

		if(num < 16) {
			/* 0..15: store directly */
			table[i] = (uint8_t) num;
			i++;
			continue;
		}

		if(num < 18) {
			/* 16..17: repeat previous code */
			uint16_t n;
			if(ARCHIVE_OK != read_bits_16(rar, p, &n))
				return ARCHIVE_EOF;

			if(num == 16) {
				n >>= 13;
				n += 3;
				skip_bits(rar, 3);
			} else {
				n >>= 9;
				n += 11;
				skip_bits(rar, 7);
			}

			if(i > 0) {
				while(n-- > 0 && i < HUFF_TABLE_SIZE) {
					table[i] = table[i - 1];
					i++;
				}
			} else {
				archive_set_error(&a->archive,
				    ARCHIVE_ERRNO_FILE_FORMAT,
				    "Unexpected error when decoding "
				    "huffman tables");
				return ARCHIVE_FATAL;
			}

			continue;
		}

		/* other codes: fill with zeroes `n` times */
		uint16_t n;
		if(ARCHIVE_OK != read_bits_16(rar, p, &n))
			return ARCHIVE_EOF;

		if(num == 18) {
			n >>= 13;
			n += 3;
			skip_bits(rar, 3);
		} else {
			n >>= 9;
			n += 11;
			skip_bits(rar, 7);
		}

		while(n-- > 0 && i < HUFF_TABLE_SIZE)
			table[i++] = 0;
	}

	ret = create_decode_tables(&table[idx], &rar->cstate.ld, HUFF_NC);
	if(ret != ARCHIVE_OK) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		     "Failed to create literal table");
		return ARCHIVE_FATAL;
	}

	idx += HUFF_NC;

	ret = create_decode_tables(&table[idx], &rar->cstate.dd, HUFF_DC);
	if(ret != ARCHIVE_OK) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Failed to create distance table");
		return ARCHIVE_FATAL;
	}

	idx += HUFF_DC;

	ret = create_decode_tables(&table[idx], &rar->cstate.ldd, HUFF_LDC);
	if(ret != ARCHIVE_OK) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Failed to create lower bits of distances table");
		return ARCHIVE_FATAL;
	}

	idx += HUFF_LDC;

	ret = create_decode_tables(&table[idx], &rar->cstate.rd, HUFF_RC);
	if(ret != ARCHIVE_OK) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
		    "Failed to create repeating distances table");
		return ARCHIVE_FATAL;
	}

	return ARCHIVE_OK;
}
