static int hid_accel_3d_probe(struct platform_device *pdev)
{
	int ret = 0;
	static const char *name = "accel_3d";
	struct iio_dev *indio_dev;
	struct accel_3d_state *accel_state;
	struct hid_sensor_hub_device *hsdev = pdev->dev.platform_data;

	indio_dev = devm_iio_device_alloc(&pdev->dev,
					  sizeof(struct accel_3d_state));
	if (indio_dev == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, indio_dev);

	accel_state = iio_priv(indio_dev);
	accel_state->common_attributes.hsdev = hsdev;
	accel_state->common_attributes.pdev = pdev;

	ret = hid_sensor_parse_common_attributes(hsdev,
					HID_USAGE_SENSOR_ACCEL_3D,
					&accel_state->common_attributes);
	if (ret) {
		dev_err(&pdev->dev, "failed to setup common attributes\n");
		return ret;
	}

	indio_dev->channels = kmemdup(accel_3d_channels,
				      sizeof(accel_3d_channels), GFP_KERNEL);
	if (!indio_dev->channels) {
		dev_err(&pdev->dev, "failed to duplicate channels\n");
		return -ENOMEM;
	}

	ret = accel_3d_parse_report(pdev, hsdev,
				    (struct iio_chan_spec *)indio_dev->channels,
				    HID_USAGE_SENSOR_ACCEL_3D, accel_state);
	if (ret) {
		dev_err(&pdev->dev, "failed to setup attributes\n");
		goto error_free_dev_mem;
	}

	indio_dev->num_channels = ARRAY_SIZE(accel_3d_channels);
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->info = &accel_3d_info;
	indio_dev->name = name;
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = iio_triggered_buffer_setup(indio_dev, &iio_pollfunc_store_time,
		NULL, NULL);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize trigger buffer\n");
		goto error_free_dev_mem;
	}
	atomic_set(&accel_state->common_attributes.data_ready, 0);
	ret = hid_sensor_setup_trigger(indio_dev, name,
					&accel_state->common_attributes);
	if (ret < 0) {
		dev_err(&pdev->dev, "trigger setup failed\n");
		goto error_unreg_buffer_funcs;
	}

	ret = iio_device_register(indio_dev);
	if (ret) {
		dev_err(&pdev->dev, "device register failed\n");
		goto error_remove_trigger;
	}

	accel_state->callbacks.send_event = accel_3d_proc_event;
	accel_state->callbacks.capture_sample = accel_3d_capture_sample;
	accel_state->callbacks.pdev = pdev;
	ret = sensor_hub_register_callback(hsdev, HID_USAGE_SENSOR_ACCEL_3D,
					&accel_state->callbacks);
	if (ret < 0) {
		dev_err(&pdev->dev, "callback reg failed\n");
		goto error_iio_unreg;
	}

	return ret;

error_iio_unreg:
	iio_device_unregister(indio_dev);
error_remove_trigger:
	hid_sensor_remove_trigger(&accel_state->common_attributes);
error_unreg_buffer_funcs:
	iio_triggered_buffer_cleanup(indio_dev);
error_free_dev_mem:
	kfree(indio_dev->channels);
	return ret;
}