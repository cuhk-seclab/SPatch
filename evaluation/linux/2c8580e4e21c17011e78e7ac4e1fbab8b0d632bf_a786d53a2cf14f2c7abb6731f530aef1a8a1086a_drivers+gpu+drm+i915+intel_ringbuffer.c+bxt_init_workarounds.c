static int bxt_init_workarounds(struct intel_engine_cs *ring)
{
	int ret;
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	ret = gen9_init_workarounds(ring);
	if (ret)
		return ret;

	/* WaStoreMultiplePTEenable:bxt */
	/* This is a requirement according to Hardware specification */
	if (IS_BXT_REVID(dev, 0, BXT_REVID_A1))
		I915_WRITE(TILECTL, I915_READ(TILECTL) | TILECTL_TLBPF);

	/* WaSetClckGatingDisableMedia:bxt */
	if (IS_BXT_REVID(dev, 0, BXT_REVID_A1)) {
		I915_WRITE(GEN7_MISCCPCTL, (I915_READ(GEN7_MISCCPCTL) &
					    ~GEN8_DOP_CLOCK_GATE_MEDIA_ENABLE));
	}

	/* WaDisableThreadStallDopClockGating:bxt */
	WA_SET_BIT_MASKED(GEN8_ROW_CHICKEN,
			  STALL_DOP_GATING_DISABLE);

	/* WaDisableSbeCacheDispatchPortSharing:bxt */
	if (IS_BXT_REVID(dev, 0, BXT_REVID_B0)) {
		WA_SET_BIT_MASKED(
			GEN7_HALF_SLICE_CHICKEN1,
			GEN7_SBE_SS_CACHE_DISPATCH_PORT_SHARING_DISABLE);
	}

	/* WaDisableObjectLevelPreemptionForTrifanOrPolygon:bxt */
	/* WaDisableObjectLevelPreemptionForInstancedDraw:bxt */
	/* WaDisableObjectLevelPreemtionForInstanceId:bxt */
	/* WaDisableLSQCROPERFforOCL:bxt */
	if (IS_BXT_REVID(dev, 0, BXT_REVID_A1)) {
		ret = wa_ring_whitelist_reg(ring, GEN9_CS_DEBUG_MODE1);
		if (ret)
			return ret;

		ret = wa_ring_whitelist_reg(ring, GEN8_L3SQCREG4);
		if (ret)
			return ret;
	}

	return 0;
}
