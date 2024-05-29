static enum drm_connector_status qxl_conn_detect(
			struct drm_connector *connector,
			bool force)
{
	struct qxl_output *output =
		drm_connector_to_qxl_output(connector);
	struct drm_device *ddev = connector->dev;
	struct qxl_device *qdev = ddev->dev_private;
	bool connected = false;

	/* The first monitor is always connected */
	if (!qdev->client_monitors_config) {
		if (output->index == 0)
			connected = true;
	} else
		connected = qdev->client_monitors_config->count > output->index &&
		     qxl_head_enabled(&qdev->client_monitors_config->heads[output->index]);

	DRM_DEBUG("#%d connected: %d\n", output->index, connected);
	if (!connected)
		qxl_monitors_config_set(qdev, output->index, 0, 0, 0, 0, 0);

	return connected ? connector_status_connected
			 : connector_status_disconnected;
}
