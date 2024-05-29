static void free_filters(struct rar5* rar) {
    struct cdeque* d = &rar->cstate.filters;

    /* Free any remaining filters. All filters should be naturally consumed by
     * the unpacking function, so remaining filters after unpacking normally
     * mean that unpacking wasn't successful. But still of course we shouldn't
     * leak memory in such case. */

    /* cdeque_size() is a fast operation, so we can use it as a loop
     * expression. */
    while(cdeque_size(d) > 0) {
        struct filter_info* f = NULL;

        /* Pop_front will also decrease the collection's size. */
        if(CDE_OK == cdeque_pop_front(d, cdeque_filter_p(&f)) && f != NULL)
            free(f);
    }

    cdeque_clear(d);

    /* Also clear out the variables needed for sanity checking. */
    rar->cstate.last_block_start = 0;
    rar->cstate.last_block_length = 0;
}
