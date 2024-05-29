static void radeon_atom_encoder_prepare(struct drm_encoder *encoder)
{
	struct radeon_device *rdev = encoder->dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_connector *connector = radeon_get_connector_for_encoder(encoder);

	if ((radeon_encoder->active_device &
	     (ATOM_DEVICE_DFP_SUPPORT | ATOM_DEVICE_LCD_SUPPORT)) ||
	    (radeon_encoder_get_dp_bridge_encoder_id(encoder) !=
	     ENCODER_OBJECT_ID_NONE)) {
		struct radeon_encoder_atom_dig *dig = radeon_encoder->enc_priv;
		if (dig) {
			dig->dig_encoder = radeon_atom_pick_dig_encoder(encoder);
			if (radeon_encoder->active_device & ATOM_DEVICE_DFP_SUPPORT) {
				if (rdev->family >= CHIP_R600)
					dig->afmt = rdev->mode_info.afmt[dig->dig_encoder];
				else
					/* RS600/690/740 have only 1 afmt block */
					dig->afmt = rdev->mode_info.afmt[0];
			}
		}
	}

	radeon_atom_output_lock(encoder, true);

	if (connector) {
		struct radeon_connector *radeon_connector = to_radeon_connector(connector);

		/* select the clock/data port if it uses a router */
		if (radeon_connector->router.cd_valid)
			radeon_router_select_cd_port(radeon_connector);

		/* turn eDP panel on for mode set */
		if (connector->connector_type == DRM_MODE_CONNECTOR_eDP)
			atombios_set_edp_panel_power(connector,
						     ATOM_TRANSMITTER_ACTION_POWER_ON);
	}

	/* this is needed for the pll/ss setup to work correctly in some cases */
	atombios_set_encoder_crtc_source(encoder);
	/* set up the FMT blocks */
	if (ASIC_IS_DCE8(rdev))
		dce8_program_fmt(encoder);
	else if (ASIC_IS_DCE4(rdev))
		dce4_program_fmt(encoder);
	else if (ASIC_IS_DCE3(rdev))
		dce3_program_fmt(encoder);
	else if (ASIC_IS_AVIVO(rdev))
		avivo_program_fmt(encoder);
}
