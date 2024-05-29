static void valleyview_pipestat_irq_handler(struct drm_device *dev, u32 iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 pipe_stats[I915_MAX_PIPES] = { };
	int pipe;

	spin_lock(&dev_priv->irq_lock);

	if (!dev_priv->display_irqs_enabled) {
		spin_unlock(&dev_priv->irq_lock);
		return;
	}

	for_each_pipe(dev_priv, pipe) {
		i915_reg_t reg;
		u32 mask, iir_bit = 0;

		/*
		 * PIPESTAT bits get signalled even when the interrupt is
		 * disabled with the mask bits, and some of the status bits do
		 * not generate interrupts at all (like the underrun bit). Hence
		 * we need to be careful that we only handle what we want to
		 * handle.
		 */

		/* fifo underruns are filterered in the underrun handler. */
		mask = PIPE_FIFO_UNDERRUN_STATUS;

		switch (pipe) {
		case PIPE_A:
			iir_bit = I915_DISPLAY_PIPE_A_EVENT_INTERRUPT;
			break;
		case PIPE_B:
			iir_bit = I915_DISPLAY_PIPE_B_EVENT_INTERRUPT;
			break;
		case PIPE_C:
			iir_bit = I915_DISPLAY_PIPE_C_EVENT_INTERRUPT;
			break;
		}
		if (iir & iir_bit)
			mask |= dev_priv->pipestat_irq_mask[pipe];

		if (!mask)
			continue;

		reg = PIPESTAT(pipe);
		mask |= PIPESTAT_INT_ENABLE_MASK;
		pipe_stats[pipe] = I915_READ(reg) & mask;

		/*
		 * Clear the PIPE*STAT regs before the IIR
		 */
		if (pipe_stats[pipe] & (PIPE_FIFO_UNDERRUN_STATUS |
					PIPESTAT_INT_STATUS_MASK))
			I915_WRITE(reg, pipe_stats[pipe]);
	}
	spin_unlock(&dev_priv->irq_lock);

	for_each_pipe(dev_priv, pipe) {
		if (pipe_stats[pipe] & PIPE_START_VBLANK_INTERRUPT_STATUS &&
		    intel_pipe_handle_vblank(dev, pipe))
			intel_check_page_flip(dev, pipe);

		if (pipe_stats[pipe] & PLANE_FLIP_DONE_INT_STATUS_VLV) {
			intel_prepare_page_flip(dev, pipe);
			intel_finish_page_flip(dev, pipe);
		}

		if (pipe_stats[pipe] & PIPE_CRC_DONE_INTERRUPT_STATUS)
			i9xx_pipe_crc_irq_handler(dev, pipe);

		if (pipe_stats[pipe] & PIPE_FIFO_UNDERRUN_STATUS)
			intel_cpu_fifo_underrun_irq_handler(dev_priv, pipe);
	}

	if (pipe_stats[0] & PIPE_GMBUS_INTERRUPT_STATUS)
		gmbus_irq_handler(dev);
}
