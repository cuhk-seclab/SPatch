static bool vlv_c0_above(struct drm_i915_private *dev_priv,
			 const struct intel_rps_ei *old,
			 const struct intel_rps_ei *now,
			 int threshold)
{
	u64 time, c0;
	unsigned int mul = 100;

	if (old->cz_clock == 0)
		return false;

	if (I915_READ(VLV_COUNTER_CONTROL) & VLV_COUNT_RANGE_HIGH)
		mul <<= 8;

	time = now->cz_clock - old->cz_clock;
	time *= threshold * dev_priv->mem_freq;

	/* Workload can be split between render + media, e.g. SwapBuffers
	 * being blitted in X after being rendered in mesa. To account for
	 * this we need to combine both engines into our activity counter.
	 */
	c0 = now->render_c0 - old->render_c0;
	c0 += now->media_c0 - old->media_c0;
	c0 *= mul * VLV_CZ_CLOCK_TO_MILLI_SEC;

	return c0 >= time;
}
