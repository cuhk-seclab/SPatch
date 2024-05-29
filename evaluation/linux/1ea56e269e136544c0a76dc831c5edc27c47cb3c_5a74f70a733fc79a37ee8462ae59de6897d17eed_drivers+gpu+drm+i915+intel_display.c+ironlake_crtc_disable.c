static void ironlake_crtc_disable(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	struct intel_encoder *encoder;
	int pipe = intel_crtc->pipe;
	u32 reg, temp;

	if (!intel_crtc->active)
		return;

	for_each_encoder_on_crtc(dev, crtc, encoder)
		encoder->disable(encoder);

	drm_crtc_vblank_off(crtc);
	assert_vblank_disabled(crtc);

	if (intel_crtc->config->has_pch_encoder)
		intel_set_pch_fifo_underrun_reporting(dev_priv, pipe, false);

	intel_disable_pipe(intel_crtc);

	ironlake_pfit_disable(intel_crtc);

	if (intel_crtc->config->has_pch_encoder)
		ironlake_fdi_disable(crtc);

	for_each_encoder_on_crtc(dev, crtc, encoder)
		if (encoder->post_disable)
			encoder->post_disable(encoder);

	if (intel_crtc->config->has_pch_encoder) {
		ironlake_disable_pch_transcoder(dev_priv, pipe);

		if (HAS_PCH_CPT(dev)) {
			/* disable TRANS_DP_CTL */
			reg = TRANS_DP_CTL(pipe);
			temp = I915_READ(reg);
			temp &= ~(TRANS_DP_OUTPUT_ENABLE |
				  TRANS_DP_PORT_SEL_MASK);
			temp |= TRANS_DP_PORT_SEL_NONE;
			I915_WRITE(reg, temp);

			/* disable DPLL_SEL */
			temp = I915_READ(PCH_DPLL_SEL);
			temp &= ~(TRANS_DPLL_ENABLE(pipe) | TRANS_DPLLB_SEL(pipe));
			I915_WRITE(PCH_DPLL_SEL, temp);
		}

		/* disable PCH DPLL */
		intel_disable_shared_dpll(intel_crtc);

		ironlake_fdi_pll_disable(intel_crtc);
	}

	intel_crtc->active = false;
	intel_update_watermarks(crtc);

	mutex_lock(&dev->struct_mutex);
	intel_fbc_update(dev);
	mutex_unlock(&dev->struct_mutex);
}
