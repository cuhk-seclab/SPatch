static struct drm_dp_mst_branch *drm_dp_get_mst_branch_device(struct drm_dp_mst_topology_mgr *mgr,
							       u8 lct, u8 *rad)
{
	struct drm_dp_mst_branch *mstb;
	struct drm_dp_mst_port *port;
	int i;
	/* find the port by iterating down */

	mutex_lock(&mgr->lock);
	mstb = mgr->mst_primary;

	for (i = 0; i < lct - 1; i++) {
		int shift = (i % 2) ? 0 : 4;
		int port_num = rad[i / 2] >> shift;

		list_for_each_entry(port, &mstb->ports, next) {
			if (port->port_num == port_num) {
				if (!port->mstb) {
					DRM_ERROR("failed to lookup MSTB with lct %d, rad %02x\n", lct, rad[0]);
					return NULL;
				}

				mstb = port->mstb;
				break;
			}
		}
	}
	kref_get(&mstb->kref);
out:
	mutex_unlock(&mgr->lock);
	return mstb;
}
