static int run_e8e9_filter(struct rar5* rar, struct filter_info* flt,
        int extended)
{
    const uint32_t file_size = 0x1000000;
    ssize_t i;

    const int mask = (int)rar->cstate.window_mask;
    circular_memcpy(rar->cstate.filtered_buf,
        rar->cstate.window_buf,
        mask,
        rar->cstate.solid_offset + flt->block_start,
        rar->cstate.solid_offset + flt->block_start + flt->block_length);

    for(i = 0; i < flt->block_length - 4;) {
        uint8_t b = rar->cstate.window_buf[(rar->cstate.solid_offset +
                flt->block_start + i++) & mask];

        /* 0xE8 = x86's call <relative_addr_uint32> (function call)
         * 0xE9 = x86's jmp <relative_addr_uint32> (unconditional jump) */
        if(b == 0xE8 || (extended && b == 0xE9)) {

            uint32_t addr;
            uint32_t offset = (i + flt->block_start) % file_size;

            addr = read_filter_data(rar, (uint32_t)(rar->cstate.solid_offset +
                        flt->block_start + i) & rar->cstate.window_mask);

            if(addr & 0x80000000) {
                if(((addr + offset) & 0x80000000) == 0) {
                    write_filter_data(rar, (uint32_t)i, addr + file_size);
                }
            } else {
                if((addr - file_size) & 0x80000000) {
                    uint32_t naddr = addr - offset;
                    write_filter_data(rar, (uint32_t)i, naddr);
                }
            }

            i += 4;
        }
    }

    return ARCHIVE_OK;
}
