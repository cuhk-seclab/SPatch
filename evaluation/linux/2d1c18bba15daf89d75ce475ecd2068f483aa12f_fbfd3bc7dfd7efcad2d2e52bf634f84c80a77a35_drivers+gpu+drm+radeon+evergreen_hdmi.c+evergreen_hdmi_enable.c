void evergreen_hdmi_enable(struct drm_encoder *encoder, bool enable)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_encoder_atom_dig *dig = radeon_encoder->enc_priv;

	if (!dig || !dig->afmt)
		return;

	if (enable) {
		struct drm_connector *connector = radeon_get_connector_for_encoder(encoder);

		if (connector && drm_detect_monitor_audio(radeon_connector_edid(connector))) {
			WREG32(HDMI_INFOFRAME_CONTROL0 + dig->afmt->offset,
			       HDMI_AVI_INFO_SEND | /* enable AVI info frames */
			       HDMI_AVI_INFO_CONT | /* required for audio info values to be updated */
			       HDMI_AUDIO_INFO_SEND | /* enable audio info frames (frames won't be set until audio is enabled) */
			       HDMI_AUDIO_INFO_CONT); /* required for audio info values to be updated */
			WREG32_OR(AFMT_AUDIO_PACKET_CONTROL + dig->afmt->offset,
				  AFMT_AUDIO_SAMPLE_SEND);
		} else {
			WREG32(HDMI_INFOFRAME_CONTROL0 + dig->afmt->offset,
			       HDMI_AVI_INFO_SEND | /* enable AVI info frames */
			       HDMI_AVI_INFO_CONT); /* required for audio info values to be updated */
			WREG32_AND(AFMT_AUDIO_PACKET_CONTROL + dig->afmt->offset,
				   ~AFMT_AUDIO_SAMPLE_SEND);
		}
	} else {
		WREG32_AND(AFMT_AUDIO_PACKET_CONTROL + dig->afmt->offset,
			   ~AFMT_AUDIO_SAMPLE_SEND);
		WREG32(HDMI_INFOFRAME_CONTROL0 + dig->afmt->offset, 0);
	}

	dig->afmt->enabled = enable;

	DRM_DEBUG("%sabling HDMI interface @ 0x%04X for encoder 0x%x\n",
		  enable ? "En" : "Dis", dig->afmt->offset, radeon_encoder->encoder_id);
}
