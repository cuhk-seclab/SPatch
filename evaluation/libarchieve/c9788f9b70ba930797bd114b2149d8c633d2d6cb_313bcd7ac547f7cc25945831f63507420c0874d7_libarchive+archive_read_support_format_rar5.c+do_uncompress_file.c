static int do_uncompress_file(struct archive_read* a) {
	struct rar5* rar = get_context(a);
	int ret;
	int64_t max_end_pos;

	if(!rar->cstate.initialized) {
		/* Don't perform full context reinitialization if we're
		 * processing a solid archive. */
		if(!rar->main.solid || !rar->cstate.window_buf) {
			init_unpack(rar);
		}

		rar->cstate.initialized = 1;
	}

	/* Don't allow extraction if window_size is invalid. */
	if(rar->cstate.window_size == 0) {
		archive_set_error(&a->archive,
			ARCHIVE_ERRNO_FILE_FORMAT,
			"Invalid window size declaration in this file");

		/* This should never happen in valid files. */
		return ARCHIVE_FATAL;
	}

	if(rar->cstate.all_filters_applied == 1) {
		/* We use while(1) here, but standard case allows for just 1
		 * iteration. The loop will iterate if process_block() didn't
		 * generate any data at all. This can happen if the block
		 * contains only filter definitions (this is common in big
		 * files). */
		while(1) {
			ret = process_block(a);
			if(ret == ARCHIVE_EOF || ret == ARCHIVE_FATAL)
				return ret;

			if(rar->cstate.last_write_ptr ==
			    rar->cstate.write_ptr) {
				/* The block didn't generate any new data,
				 * so just process a new block. */
				continue;
			}

			/* The block has generated some new data, so break
			 * the loop. */
			break;
		}
	}

	/* Try to run filters. If filters won't be applied, it means that
	 * insufficient data was generated. */
	ret = apply_filters(a);
	if(ret == ARCHIVE_RETRY) {
		return ARCHIVE_OK;
	} else if(ret == ARCHIVE_FATAL) {
		return ARCHIVE_FATAL;
	}

	/* If apply_filters() will return ARCHIVE_OK, we can continue here. */

	if(cdeque_size(&rar->cstate.filters) > 0) {
		/* Check if we can write something before hitting first
		 * filter. */
		struct filter_info* flt;

		/* Get the block_start offset from the first filter. */
		if(CDE_OK != cdeque_front(&rar->cstate.filters,
		    cdeque_filter_p(&flt)))
		{
			archive_set_error(&a->archive,
			    ARCHIVE_ERRNO_PROGRAMMER,
			    "Can't read first filter");
			return ARCHIVE_FATAL;
		}

		max_end_pos = rar5_min(flt->block_start,
		    rar->cstate.write_ptr);
	} else {
		/* There are no filters defined, or all filters were applied.
		 * This means we can just store the data without any
		 * postprocessing. */
		max_end_pos = rar->cstate.write_ptr;
	}

	if(max_end_pos == rar->cstate.last_write_ptr) {
		/* We can't write anything yet. The block uncompression
		 * function did not generate enough data, and no filter can be
		 * applied. At the same time we don't have any data that can be
		 *  stored without filter postprocessing. This means we need to
		 *  wait for more data to be generated, so we can apply the
		 * filters.
		 *
		 * Signal the caller that we need more data to be able to do
		 * anything.
		 */
		return ARCHIVE_RETRY;
	} else {
		/* We can write the data before hitting the first filter.
		 * So let's do it. The push_window_data() function will
		 * effectively return the selected data block to the user
		 * application. */
		push_window_data(a, rar, rar->cstate.last_write_ptr,
		    max_end_pos);
		rar->cstate.last_write_ptr = max_end_pos;
	}

	return ARCHIVE_OK;
}
