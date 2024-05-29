static void
intel_dp_set_signal_levels(struct intel_dp *intel_dp, uint32_t *DP)
{
	struct intel_digital_port *intel_dig_port = dp_to_dig_port(intel_dp);
	enum port port = intel_dig_port->port;
	struct drm_device *dev = intel_dig_port->base.base.dev;
	uint32_t signal_levels, mask;
	uint8_t train_set = intel_dp->train_set[0];

		signal_levels = ddi_signal_levels(intel_dp);

		if (IS_BROXTON(dev))
			signal_levels = 0;
		else
	if (IS_BROXTON(dev)) {
		mask = DDI_BUF_EMP_MASK;
	} else if (IS_CHERRYVIEW(dev)) {
		signal_levels = chv_signal_levels(intel_dp);
		mask = 0;
	} else if (IS_VALLEYVIEW(dev)) {
		signal_levels = vlv_signal_levels(intel_dp);
		mask = 0;
	} else if (IS_GEN7(dev) && port == PORT_A) {
		signal_levels = gen7_edp_signal_levels(train_set);
		mask = EDP_LINK_TRAIN_VOL_EMP_MASK_IVB;
	} else if (IS_GEN6(dev) && port == PORT_A) {
		signal_levels = gen6_edp_signal_levels(train_set);
		mask = EDP_LINK_TRAIN_VOL_EMP_MASK_SNB;
	} else {
		signal_levels = gen4_signal_levels(train_set);
		mask = DP_VOLTAGE_MASK | DP_PRE_EMPHASIS_MASK;
	}

	if (mask)
		DRM_DEBUG_KMS("Using signal levels %08x\n", signal_levels);

	DRM_DEBUG_KMS("Using vswing level %d\n",
		train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
	DRM_DEBUG_KMS("Using pre-emphasis level %d\n",
		(train_set & DP_TRAIN_PRE_EMPHASIS_MASK) >>
			DP_TRAIN_PRE_EMPHASIS_SHIFT);

	*DP = (*DP & ~mask) | signal_levels;
}
