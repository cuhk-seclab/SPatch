int r100_irq_set(struct radeon_device *rdev)
{
	uint32_t tmp = 0;

	if (!rdev->irq.installed) {
		WARN(1, "Can't enable IRQ/MSI because no handler is installed\n");
		WREG32(R_000040_GEN_INT_CNTL, 0);
		return -EINVAL;
	}
	if (atomic_read(&rdev->irq.ring_int[RADEON_RING_TYPE_GFX_INDEX])) {
		tmp |= RADEON_SW_INT_ENABLE;
	}
	if (rdev->irq.crtc_vblank_int[0] ||
	    atomic_read(&rdev->irq.pflip[0])) {
		tmp |= RADEON_CRTC_VBLANK_MASK;
	}
	if (rdev->irq.crtc_vblank_int[1] ||
	    atomic_read(&rdev->irq.pflip[1])) {
		tmp |= RADEON_CRTC2_VBLANK_MASK;
	}
	if (rdev->irq.hpd[0]) {
		tmp |= RADEON_FP_DETECT_MASK;
	}
	if (rdev->irq.hpd[1]) {
		tmp |= RADEON_FP2_DETECT_MASK;
	}
	WREG32(RADEON_GEN_INT_CNTL, tmp);

	/* read back to post the write */
	RREG32(RADEON_GEN_INT_CNTL);

	return 0;
}
