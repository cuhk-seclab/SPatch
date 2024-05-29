static int bmc150_accel_write_event_config(struct iio_dev *indio_dev,
					   const struct iio_chan_spec *chan,
					   enum iio_event_type type,
					   enum iio_event_direction dir,
					   int state)
{
	struct bmc150_accel_data *data = iio_priv(indio_dev);
	int ret;

	if (state == data->ev_enable_state)
		return 0;

	mutex_lock(&data->mutex);

	if (!state && data->motion_trigger_on) {
		data->ev_enable_state = 0;
		mutex_unlock(&data->mutex);
		return 0;
	}

	ret =  bmc150_accel_setup_any_motion_interrupt(data, state);
	if (ret < 0) {
		mutex_unlock(&data->mutex);
		return ret;
	}

	data->ev_enable_state = state;
	mutex_unlock(&data->mutex);

	return 0;
}
