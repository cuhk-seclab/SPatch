static void intel_prepare_ddi_buffers(struct drm_device *dev, enum port port,
				      bool supports_hdmi)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 reg;
	int i, n_hdmi_entries, n_dp_entries, n_edp_entries, hdmi_default_entry,
	    size;
	int hdmi_level = dev_priv->vbt.ddi_port_info[port].hdmi_level_shift;
	const struct ddi_buf_trans *ddi_translations_fdi;
	const struct ddi_buf_trans *ddi_translations_dp;
	const struct ddi_buf_trans *ddi_translations_edp;
	const struct ddi_buf_trans *ddi_translations_hdmi;
	const struct ddi_buf_trans *ddi_translations;

	if (IS_BROXTON(dev)) {
		if (!supports_hdmi)
			return;

		/* Vswing programming for HDMI */
		bxt_ddi_vswing_sequence(dev, hdmi_level, port,
					INTEL_OUTPUT_HDMI);
		return;
	} else if (IS_SKYLAKE(dev)) {
		ddi_translations_dp =
				skl_get_buf_trans_dp(dev, &n_dp_entries);
		ddi_translations_edp =
				skl_get_buf_trans_edp(dev, &n_edp_entries);
		ddi_translations_hdmi =
				skl_get_buf_trans_hdmi(dev, &n_hdmi_entries);
		hdmi_default_entry = 8;
	} else if (IS_BROADWELL(dev)) {
		ddi_translations_fdi = bdw_ddi_translations_fdi;
		ddi_translations_dp = bdw_ddi_translations_dp;
		ddi_translations_edp = bdw_ddi_translations_edp;
		ddi_translations_hdmi = bdw_ddi_translations_hdmi;
		n_edp_entries = ARRAY_SIZE(bdw_ddi_translations_edp);
		n_dp_entries = ARRAY_SIZE(bdw_ddi_translations_dp);
		n_hdmi_entries = ARRAY_SIZE(bdw_ddi_translations_hdmi);
		hdmi_default_entry = 7;
	} else if (IS_HASWELL(dev)) {
		ddi_translations_fdi = hsw_ddi_translations_fdi;
		ddi_translations_dp = hsw_ddi_translations_dp;
		ddi_translations_edp = hsw_ddi_translations_dp;
		ddi_translations_hdmi = hsw_ddi_translations_hdmi;
		n_dp_entries = n_edp_entries = ARRAY_SIZE(hsw_ddi_translations_dp);
		n_hdmi_entries = ARRAY_SIZE(hsw_ddi_translations_hdmi);
		hdmi_default_entry = 6;
	} else {
		WARN(1, "ddi translation table missing\n");
		ddi_translations_edp = bdw_ddi_translations_dp;
		ddi_translations_fdi = bdw_ddi_translations_fdi;
		ddi_translations_dp = bdw_ddi_translations_dp;
		ddi_translations_hdmi = bdw_ddi_translations_hdmi;
		n_edp_entries = ARRAY_SIZE(bdw_ddi_translations_edp);
		n_dp_entries = ARRAY_SIZE(bdw_ddi_translations_dp);
		n_hdmi_entries = ARRAY_SIZE(bdw_ddi_translations_hdmi);
		hdmi_default_entry = 7;
	}

	switch (port) {
	case PORT_A:
		ddi_translations = ddi_translations_edp;
		size = n_edp_entries;
		break;
	case PORT_B:
	case PORT_C:
		ddi_translations = ddi_translations_dp;
		size = n_dp_entries;
		break;
	case PORT_D:
		if (intel_dp_is_edp(dev, PORT_D)) {
			ddi_translations = ddi_translations_edp;
			size = n_edp_entries;
		} else {
			ddi_translations = ddi_translations_dp;
			size = n_dp_entries;
		}
		break;
	case PORT_E:
		if (ddi_translations_fdi)
			ddi_translations = ddi_translations_fdi;
		else
			ddi_translations = ddi_translations_dp;
		size = n_dp_entries;
		break;
	default:
		BUG();
	}

	for (i = 0, reg = DDI_BUF_TRANS(port); i < size; i++) {
		I915_WRITE(reg, ddi_translations[i].trans1);
		reg += 4;
		I915_WRITE(reg, ddi_translations[i].trans2);
		reg += 4;
	}

	if (!supports_hdmi)
		return;

	/* Choose a good default if VBT is badly populated */
	if (hdmi_level == HDMI_LEVEL_SHIFT_UNKNOWN ||
	    hdmi_level >= n_hdmi_entries)
		hdmi_level = hdmi_default_entry;

	/* Entry 9 is for HDMI: */
	I915_WRITE(reg, ddi_translations_hdmi[hdmi_level].trans1);
	reg += 4;
	I915_WRITE(reg, ddi_translations_hdmi[hdmi_level].trans2);
	reg += 4;
}
