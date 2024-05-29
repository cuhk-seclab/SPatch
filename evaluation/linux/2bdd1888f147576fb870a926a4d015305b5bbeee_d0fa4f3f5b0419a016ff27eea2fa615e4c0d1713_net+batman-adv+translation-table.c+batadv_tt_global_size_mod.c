static void batadv_tt_global_size_mod(struct batadv_orig_node *orig_node,
				      unsigned short vid, int v)
{
	struct batadv_orig_node_vlan *vlan;

	vlan = batadv_orig_node_vlan_new(orig_node, vid);
	if (!vlan)
		return;

	if (atomic_add_return(v, &vlan->tt.num_entries) == 0) {
		spin_lock_bh(&orig_node->vlan_list_lock);
		hlist_del_rcu(&vlan->list);
		spin_unlock_bh(&orig_node->vlan_list_lock);
		batadv_orig_node_vlan_free_ref(vlan);
	}

	batadv_orig_node_vlan_free_ref(vlan);
}
