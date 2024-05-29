static void intel_plane_atomic_update(struct drm_plane *plane,
				      struct drm_plane_state *old_state)
{
	struct intel_plane *intel_plane = to_intel_plane(plane);
	struct intel_plane_state *intel_state =
		to_intel_plane_state(plane->state);

	intel_plane->commit_plane(plane, intel_state);
}
