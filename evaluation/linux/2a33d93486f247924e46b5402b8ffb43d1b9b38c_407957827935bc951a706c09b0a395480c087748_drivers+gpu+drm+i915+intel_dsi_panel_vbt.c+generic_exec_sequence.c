static void generic_exec_sequence(struct intel_dsi *intel_dsi, const u8 *data)
{
	struct drm_i915_private *dev_priv = to_i915(intel_dsi->base.base.dev);
	fn_mipi_elem_exec mipi_elem_exec;

	if (!data)
		return;

	DRM_DEBUG_DRIVER("Starting MIPI sequence %u - %s\n",
			 *data, sequence_name(*data));

	/* go to the first element of the sequence */
	data++;

	/* Skip Size of Sequence. */
	if (dev_priv->vbt.dsi.seq_version >= 3)
		data += 4;

	/* parse each byte till we reach end of sequence byte - 0x00 */
	while (1) {
		u8 operation_byte = *data++;

		if (operation_byte == MIPI_SEQ_ELEM_END)
			break;

		if (operation_byte < ARRAY_SIZE(exec_elem))
			mipi_elem_exec = exec_elem[operation_byte];
		else
			mipi_elem_exec = NULL;

		/* Size of Operation. */
		if (dev_priv->vbt.dsi.seq_version >= 3)
			operation_size = *data++;

		if (mipi_elem_exec) {
			data = mipi_elem_exec(intel_dsi, data);
		} else if (operation_size) {
			/* We have size, skip. */
			DRM_DEBUG_KMS("Unsupported MIPI operation byte %u\n",
				      operation_byte);
			data += operation_size;
		} else {
		if (operation_byte >= ARRAY_SIZE(exec_elem) ||
		    !exec_elem[operation_byte]) {
			DRM_ERROR("Unsupported MIPI operation byte %u\n",
				  operation_byte);
			return;
		}
		mipi_elem_exec = exec_elem[operation_byte];
			break;
	}
}
