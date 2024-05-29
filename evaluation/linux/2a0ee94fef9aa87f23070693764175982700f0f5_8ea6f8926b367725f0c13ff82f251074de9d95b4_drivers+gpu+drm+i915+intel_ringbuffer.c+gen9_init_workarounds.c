static int gen9_init_workarounds(struct intel_engine_cs *ring)
{
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t tmp;

	/* WaDisablePartialInstShootdown:skl,bxt */
	WA_SET_BIT_MASKED(GEN8_ROW_CHICKEN,
			  PARTIAL_INSTRUCTION_SHOOTDOWN_DISABLE);

	/* Syncing dependencies between camera and graphics:skl,bxt */
	WA_SET_BIT_MASKED(HALF_SLICE_CHICKEN3,
			  GEN9_DISABLE_OCL_OOB_SUPPRESS_LOGIC);

	if ((IS_SKYLAKE(dev) && (INTEL_REVID(dev) == SKL_REVID_A0 ||
	    INTEL_REVID(dev) == SKL_REVID_B0)) ||
	    (IS_BROXTON(dev) && INTEL_REVID(dev) < BXT_REVID_B0)) {
		/* WaDisableDgMirrorFixInHalfSliceChicken5:skl,bxt */
		WA_CLR_BIT_MASKED(GEN9_HALF_SLICE_CHICKEN5,
				  GEN9_DG_MIRROR_FIX_ENABLE);
	}

	if ((IS_SKYLAKE(dev) && INTEL_REVID(dev) <= SKL_REVID_B0) ||
	    (IS_BROXTON(dev) && INTEL_REVID(dev) < BXT_REVID_B0)) {
		/* WaSetDisablePixMaskCammingAndRhwoInCommonSliceChicken:skl,bxt */
		WA_SET_BIT_MASKED(GEN7_COMMON_SLICE_CHICKEN1,
				  GEN9_RHWO_OPTIMIZATION_DISABLE);
		WA_SET_BIT_MASKED(GEN9_SLICE_COMMON_ECO_CHICKEN0,
				  DISABLE_PIXEL_MASK_CAMMING);
	}

	if ((IS_SKYLAKE(dev) && INTEL_REVID(dev) >= SKL_REVID_C0) ||
	    IS_BROXTON(dev)) {
		/* WaEnableYV12BugFixInHalfSliceChicken7:skl,bxt */
		WA_SET_BIT_MASKED(GEN9_HALF_SLICE_CHICKEN7,
				  GEN9_ENABLE_YV12_BUGFIX);
	}

	/* Wa4x4STCOptimizationDisable:skl,bxt */
	WA_SET_BIT_MASKED(CACHE_MODE_1, GEN8_4x4_STC_OPTIMIZATION_DISABLE);

	/* WaDisablePartialResolveInVc:skl,bxt */
	WA_SET_BIT_MASKED(CACHE_MODE_1, GEN9_PARTIAL_RESOLVE_IN_VC_DISABLE);

	/* WaCcsTlbPrefetchDisable:skl,bxt */
	WA_CLR_BIT_MASKED(GEN9_HALF_SLICE_CHICKEN5,
			  GEN9_CCS_TLB_PREFETCH_ENABLE);

	/* WaDisableMaskBasedCammingInRCC:skl,bxt */
	if ((IS_SKYLAKE(dev) && INTEL_REVID(dev) == SKL_REVID_C0) ||
	    (IS_BROXTON(dev) && INTEL_REVID(dev) < BXT_REVID_B0))
		WA_SET_BIT_MASKED(SLICE_ECO_CHICKEN0,
				  PIXEL_MASK_CAMMING_DISABLE);

	/* WaForceContextSaveRestoreNonCoherent:skl,bxt */
	tmp = HDC_FORCE_CONTEXT_SAVE_RESTORE_NON_COHERENT;
	if ((IS_SKYLAKE(dev) && INTEL_REVID(dev) == SKL_REVID_F0) ||
	    (IS_BROXTON(dev) && INTEL_REVID(dev) >= BXT_REVID_B0))
		tmp |= HDC_FORCE_CSR_NON_COHERENT_OVR_DISABLE;
	WA_SET_BIT_MASKED(HDC_CHICKEN0, tmp);

	return 0;
}
