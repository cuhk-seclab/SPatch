struct batadv_orig_node_vlan *
batadv_orig_node_vlan_get(struct batadv_orig_node *orig_node,
			  unsigned short vid)
{
	struct batadv_orig_node_vlan *vlan = NULL, *tmp;

	rcu_read_lock();
	hlist_for_each_entry_rcu(tmp, &orig_node->vlan_list, list) {
		if (tmp->vid != vid)
			continue;

		if (!atomic_inc_not_zero(&tmp->refcount))
			continue;

		vlan = tmp;

		break;
	}
	rcu_read_unlock();

	return vlan;
}
