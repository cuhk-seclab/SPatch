static void batadv_mcast_want_ipv4_update(struct batadv_priv *bat_priv,
					  struct batadv_orig_node *orig,
					  u8 mcast_flags)
{
	struct hlist_node *node = &orig->mcast_want_all_ipv4_node;
	struct hlist_head *head = &bat_priv->mcast.want_all_ipv4_list;

	lockdep_assert_held(&orig->mcast_handler_lock);

	/* switched from flag unset to set */
	if (mcast_flags & BATADV_MCAST_WANT_ALL_IPV4 &&
	    !(orig->mcast_flags & BATADV_MCAST_WANT_ALL_IPV4)) {
		atomic_inc(&bat_priv->mcast.num_want_all_ipv4);

		spin_lock_bh(&bat_priv->mcast.want_lists_lock);
		/* flag checks above + mcast_handler_lock prevents this */
		WARN_ON(!hlist_unhashed(node));

		hlist_add_head_rcu(node, head);
		spin_unlock_bh(&bat_priv->mcast.want_lists_lock);
	/* switched from flag set to unset */
	} else if (!(mcast_flags & BATADV_MCAST_WANT_ALL_IPV4) &&
		   orig->mcast_flags & BATADV_MCAST_WANT_ALL_IPV4) {
		atomic_dec(&bat_priv->mcast.num_want_all_ipv4);

		spin_lock_bh(&bat_priv->mcast.want_lists_lock);
		/* flag checks above + mcast_handler_lock prevents this */
		WARN_ON(hlist_unhashed(node));

		hlist_del_init_rcu(node);
		spin_unlock_bh(&bat_priv->mcast.want_lists_lock);
	}
}
