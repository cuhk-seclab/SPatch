static void skl_set_power_well(struct drm_i915_private *dev_priv,
			struct i915_power_well *power_well, bool enable)
{
	uint32_t tmp, fuse_status;
	uint32_t req_mask, state_mask;
	bool is_enabled, enable_requested, check_fuse_status = false;

	tmp = I915_READ(HSW_PWR_WELL_DRIVER);
	fuse_status = I915_READ(SKL_FUSE_STATUS);

	switch (power_well->data) {
	case SKL_DISP_PW_1:
		if (wait_for((I915_READ(SKL_FUSE_STATUS) &
			SKL_FUSE_PG0_DIST_STATUS), 1)) {
			DRM_ERROR("PG0 not enabled\n");
			return;
		}
		break;
	case SKL_DISP_PW_2:
		if (!(fuse_status & SKL_FUSE_PG1_DIST_STATUS)) {
			DRM_ERROR("PG1 in disabled state\n");
			return;
		}
		break;
	case SKL_DISP_PW_DDI_A_E:
	case SKL_DISP_PW_DDI_B:
	case SKL_DISP_PW_DDI_C:
	case SKL_DISP_PW_DDI_D:
	case SKL_DISP_PW_MISC_IO:
		break;
	default:
		WARN(1, "Unknown power well %lu\n", power_well->data);
		return;
	}

	req_mask = SKL_POWER_WELL_REQ(power_well->data);
	enable_requested = tmp & req_mask;
	state_mask = SKL_POWER_WELL_STATE(power_well->data);
	is_enabled = tmp & state_mask;

	if (enable) {
		if (!enable_requested) {
			I915_WRITE(HSW_PWR_WELL_DRIVER, tmp | req_mask);
		}

		if (!is_enabled) {
			DRM_DEBUG_KMS("Enabling %s\n", power_well->name);
			if (wait_for((I915_READ(HSW_PWR_WELL_DRIVER) &
				state_mask), 1))
				DRM_ERROR("%s enable timeout\n",
					power_well->name);
			check_fuse_status = true;
		}
	} else {
		if (enable_requested) {
			I915_WRITE(HSW_PWR_WELL_DRIVER,	tmp & ~req_mask);
			POSTING_READ(HSW_PWR_WELL_DRIVER);
			DRM_DEBUG_KMS("Disabling %s\n", power_well->name);
		}
	}

	if (check_fuse_status) {
		if (power_well->data == SKL_DISP_PW_1) {
			if (wait_for((I915_READ(SKL_FUSE_STATUS) &
				SKL_FUSE_PG1_DIST_STATUS), 1))
				DRM_ERROR("PG1 distributing status timeout\n");
		} else if (power_well->data == SKL_DISP_PW_2) {
			if (wait_for((I915_READ(SKL_FUSE_STATUS) &
				SKL_FUSE_PG2_DIST_STATUS), 1))
				DRM_ERROR("PG2 distributing status timeout\n");
		}
	}
}
