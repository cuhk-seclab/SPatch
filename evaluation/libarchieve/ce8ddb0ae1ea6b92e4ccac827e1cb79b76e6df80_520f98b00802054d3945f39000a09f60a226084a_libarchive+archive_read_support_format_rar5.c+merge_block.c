static int merge_block(struct archive_read* a, ssize_t block_size,
        const uint8_t** p)
{
    struct rar5* rar = get_context(a);
    ssize_t cur_block_size, partial_offset = 0;
    const uint8_t* lp;
    int ret;

    /* Set a flag that we're in the switching mode. */
    rar->cstate.switch_multivolume = 1;

    /* Reallocate the memory which will hold the whole block. */
    if(rar->vol.push_buf)
        free((void*) rar->vol.push_buf);

    /* Increasing the allocation block by 8 is due to bit reading functions,
     * which are using additional 2 or 4 bytes. Allocating the block size
     * by exact value would make bit reader perform reads from invalid memory
     * block when reading the last byte from the buffer. */
    rar->vol.push_buf = malloc(block_size + 8);
    if(!rar->vol.push_buf) {
        archive_set_error(&a->archive, ENOMEM, "Can't allocate memory for a "
                "merge block buffer.");
        return ARCHIVE_FATAL;
    }

    /* Valgrind complains if the extension block for bit reader is not
     * initialized, so initialize it. */
    memset(&rar->vol.push_buf[block_size], 0, 8);

    /* A single block can span across multiple multivolume archive files,
     * so we use a loop here. This loop will consume enough multivolume
     * archive files until the whole block is read. */

    while(1) {
        /* Get the size of current block chunk in this multivolume archive
         * file and read it. */
        cur_block_size =
            rar5_min(rar->file.bytes_remaining, block_size - partial_offset);

        if(cur_block_size == 0) {
            archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT,
                    "Encountered block size == 0 during block merge");
            return ARCHIVE_FATAL;
        }

        if(!read_ahead(a, cur_block_size, &lp))
            return ARCHIVE_EOF;

        /* Sanity check; there should never be a situation where this function
         * reads more data than the block's size. */
        if(partial_offset + cur_block_size > block_size) {
            archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER,
                "Consumed too much data when merging blocks.");
            return ARCHIVE_FATAL;
        }

        /* Merge previous block chunk with current block chunk, or create
         * first block chunk if this is our first iteration. */
        memcpy(&rar->vol.push_buf[partial_offset], lp, cur_block_size);

        /* Advance the stream read pointer by this block chunk size. */
        if(ARCHIVE_OK != consume(a, cur_block_size))
            return ARCHIVE_EOF;

        /* Update the pointers. `partial_offset` contains information about
         * the sum of merged block chunks. */
        partial_offset += cur_block_size;
        rar->file.bytes_remaining -= cur_block_size;

        /* If `partial_offset` is the same as `block_size`, this means we've
         * merged all block chunks and we have a valid full block. */
        if(partial_offset == block_size) {
            break;
        }

        /* If we don't have any bytes to read, this means we should switch
         * to another multivolume archive file. */
        if(rar->file.bytes_remaining == 0) {
            ret = advance_multivolume(a);
            if(ret != ARCHIVE_OK)
                return ret;
        }
    }

    *p = rar->vol.push_buf;

    /* If we're here, we can resume unpacking by processing the block pointed
     * to by the `*p` memory pointer. */

    return ARCHIVE_OK;
}
