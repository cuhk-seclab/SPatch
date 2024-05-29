bool i40e_is_vsi_in_vlan(struct i40e_vsi *vsi)
{
	struct i40e_mac_filter *f;

	/* Only -1 for all the filters denotes not in vlan mode
	 * so we have to go through all the list in order to make sure
	 */
	list_for_each_entry(f, &vsi->mac_filter_list, list) {
		if (f->vlan >= 0 || vsi->info.pvid)
			return true;
	}

	return false;
}
