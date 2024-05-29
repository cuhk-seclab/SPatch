static int __iio_update_buffers(struct iio_dev *indio_dev,
		       struct iio_buffer *insert_buffer,
		       struct iio_buffer *remove_buffer)
{
	int ret;
	int success = 0;
	struct iio_buffer *buffer;
	unsigned long *compound_mask;
	const unsigned long *old_mask;

	if (insert_buffer) {
		ret = iio_buffer_request_update(indio_dev, insert_buffer);
		if (ret)
			return ret;
	}

	/* Wind down existing buffers - iff there are any */
	if (!list_empty(&indio_dev->buffer_list)) {
		if (indio_dev->setup_ops->predisable) {
			ret = indio_dev->setup_ops->predisable(indio_dev);
			if (ret)
				return ret;
		}
		indio_dev->currentmode = INDIO_DIRECT_MODE;
		if (indio_dev->setup_ops->postdisable) {
			ret = indio_dev->setup_ops->postdisable(indio_dev);
			if (ret)
				return ret;
		}
	}
	/* Keep a copy of current setup to allow roll back */
	old_mask = indio_dev->active_scan_mask;
	if (!indio_dev->available_scan_masks)
		indio_dev->active_scan_mask = NULL;

	if (remove_buffer)
		iio_buffer_deactivate(remove_buffer);
	if (insert_buffer)
		iio_buffer_activate(indio_dev, insert_buffer);

	/* If no buffers in list, we are done */
	if (list_empty(&indio_dev->buffer_list)) {
		indio_dev->currentmode = INDIO_DIRECT_MODE;
		iio_free_scan_mask(indio_dev, old_mask);
		return 0;
	}

	/* What scan mask do we actually have? */
	compound_mask = kcalloc(BITS_TO_LONGS(indio_dev->masklength),
				sizeof(long), GFP_KERNEL);
	if (compound_mask == NULL) {
		iio_free_scan_mask(indio_dev, old_mask);
		return -ENOMEM;
	}
	indio_dev->scan_timestamp = 0;

	list_for_each_entry(buffer, &indio_dev->buffer_list, buffer_list) {
		bitmap_or(compound_mask, compound_mask, buffer->scan_mask,
			  indio_dev->masklength);
		indio_dev->scan_timestamp |= buffer->scan_timestamp;
	}
	if (indio_dev->available_scan_masks) {
		indio_dev->active_scan_mask =
			iio_scan_mask_match(indio_dev->available_scan_masks,
					    indio_dev->masklength,
					    compound_mask);
		kfree(compound_mask);
		if (indio_dev->active_scan_mask == NULL) {
			/*
			 * Roll back.
			 * Note can only occur when adding a buffer.
			 */
			iio_buffer_deactivate(insert_buffer);
			if (old_mask) {
				indio_dev->active_scan_mask = old_mask;
				success = -EINVAL;
			}
			else {
				ret = -EINVAL;
				return ret;
			}
		}
	} else {
		indio_dev->active_scan_mask = compound_mask;
	}

	iio_update_demux(indio_dev);

	/* Wind up again */
	if (indio_dev->setup_ops->preenable) {
		ret = indio_dev->setup_ops->preenable(indio_dev);
		if (ret) {
			dev_dbg(&indio_dev->dev,
			       "Buffer not started: buffer preenable failed (%d)\n", ret);
			goto error_remove_inserted;
		}
	}
	indio_dev->scan_bytes =
		iio_compute_scan_bytes(indio_dev,
				       indio_dev->active_scan_mask,
				       indio_dev->scan_timestamp);
	if (indio_dev->info->update_scan_mode) {
		ret = indio_dev->info
			->update_scan_mode(indio_dev,
					   indio_dev->active_scan_mask);
		if (ret < 0) {
			dev_dbg(&indio_dev->dev,
				"Buffer not started: update scan mode failed (%d)\n",
				ret);
			goto error_run_postdisable;
		}
	}
	/* Definitely possible for devices to support both of these. */
	if ((indio_dev->modes & INDIO_BUFFER_TRIGGERED) && indio_dev->trig) {
		indio_dev->currentmode = INDIO_BUFFER_TRIGGERED;
	} else if (indio_dev->modes & INDIO_BUFFER_HARDWARE) {
		indio_dev->currentmode = INDIO_BUFFER_HARDWARE;
	} else if (indio_dev->modes & INDIO_BUFFER_SOFTWARE) {
		indio_dev->currentmode = INDIO_BUFFER_SOFTWARE;
	} else { /* Should never be reached */
		/* Can only occur on first buffer */
		if (indio_dev->modes & INDIO_BUFFER_TRIGGERED)
			dev_dbg(&indio_dev->dev, "Buffer not started: no trigger\n");
		ret = -EINVAL;
		goto error_run_postdisable;
	}

	if (indio_dev->setup_ops->postenable) {
		ret = indio_dev->setup_ops->postenable(indio_dev);
		if (ret) {
			dev_dbg(&indio_dev->dev,
			       "Buffer not started: postenable failed (%d)\n", ret);
			indio_dev->currentmode = INDIO_DIRECT_MODE;
			if (indio_dev->setup_ops->postdisable)
				indio_dev->setup_ops->postdisable(indio_dev);
			goto error_disable_all_buffers;
		}
	}

	iio_free_scan_mask(indio_dev, old_mask);

	return success;

error_disable_all_buffers:
	indio_dev->currentmode = INDIO_DIRECT_MODE;
error_run_postdisable:
	if (indio_dev->setup_ops->postdisable)
		indio_dev->setup_ops->postdisable(indio_dev);
error_remove_inserted:
	if (insert_buffer)
		iio_buffer_deactivate(insert_buffer);
	iio_free_scan_mask(indio_dev, indio_dev->active_scan_mask);
	indio_dev->active_scan_mask = old_mask;
	return ret;
}
