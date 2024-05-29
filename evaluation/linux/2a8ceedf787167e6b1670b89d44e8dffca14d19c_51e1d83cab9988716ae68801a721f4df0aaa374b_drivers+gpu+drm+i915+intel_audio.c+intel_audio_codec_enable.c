void intel_audio_codec_enable(struct intel_encoder *intel_encoder)
{
	struct drm_encoder *encoder = &intel_encoder->base;
	struct intel_crtc *crtc = to_intel_crtc(encoder->crtc);
	struct drm_display_mode *mode = &crtc->config->base.adjusted_mode;
	struct drm_connector *connector;
	struct drm_device *dev = encoder->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_audio_component *acomp = dev_priv->audio_component;
	struct intel_digital_port *intel_dig_port = enc_to_dig_port(encoder);
	enum port port = intel_dig_port->port;

	connector = drm_select_eld(encoder, mode);
	if (!connector)
		return;

	DRM_DEBUG_DRIVER("ELD on [CONNECTOR:%d:%s], [ENCODER:%d:%s]\n",
			 connector->base.id,
			 connector->name,
			 connector->encoder->base.id,
			 connector->encoder->name);

	/* ELD Conn_Type */
	connector->eld[5] &= ~(3 << 2);
	if (intel_pipe_has_type(crtc, INTEL_OUTPUT_DISPLAYPORT))
		connector->eld[5] |= (1 << 2);

	connector->eld[6] = drm_av_sync_delay(connector, mode) / 2;

	if (dev_priv->display.audio_codec_enable)
		dev_priv->display.audio_codec_enable(connector, intel_encoder, mode);

	if (acomp && acomp->audio_ops && acomp->audio_ops->pin_eld_notify)
		acomp->audio_ops->pin_eld_notify(acomp->audio_ops->audio_ptr, (int) port, 0);
}
