void
batadv_purge_outstanding_packets(struct batadv_priv *bat_priv,
				 const struct batadv_hard_iface *hard_iface)
{
	struct batadv_forw_packet *forw_packet;
	struct hlist_node *safe_tmp_node;
	bool pending;

	if (hard_iface)
		batadv_dbg(BATADV_DBG_BATMAN, bat_priv,
			   "purge_outstanding_packets(): %s\n",
			   hard_iface->net_dev->name);
	else
		batadv_dbg(BATADV_DBG_BATMAN, bat_priv,
			   "purge_outstanding_packets()\n");

	/* free bcast list */
	spin_lock_bh(&bat_priv->forw_bcast_list_lock);
	hlist_for_each_entry_safe(forw_packet, safe_tmp_node,
				  &bat_priv->forw_bcast_list, list) {
		/* if purge_outstanding_packets() was called with an argument
		 * we delete only packets belonging to the given interface
		 */
		if ((hard_iface) &&
		    (forw_packet->if_incoming != hard_iface) &&
		    (forw_packet->if_outgoing != hard_iface))
			continue;

		spin_unlock_bh(&bat_priv->forw_bcast_list_lock);

		/* batadv_send_outstanding_bcast_packet() will lock the list to
		 * delete the item from the list
		 */
		pending = cancel_delayed_work_sync(&forw_packet->delayed_work);
		spin_lock_bh(&bat_priv->forw_bcast_list_lock);

		if (pending) {
			hlist_del(&forw_packet->list);
			batadv_forw_packet_free(forw_packet);
		}
	}
	spin_unlock_bh(&bat_priv->forw_bcast_list_lock);

	/* free batman packet list */
	spin_lock_bh(&bat_priv->forw_bat_list_lock);
	hlist_for_each_entry_safe(forw_packet, safe_tmp_node,
				  &bat_priv->forw_bat_list, list) {
		/* if purge_outstanding_packets() was called with an argument
		 * we delete only packets belonging to the given interface
		 */
		if ((hard_iface) &&
		    (forw_packet->if_incoming != hard_iface) &&
		    (forw_packet->if_outgoing != hard_iface))
			continue;

		spin_unlock_bh(&bat_priv->forw_bat_list_lock);

		/* send_outstanding_bat_packet() will lock the list to
		 * delete the item from the list
		 */
		pending = cancel_delayed_work_sync(&forw_packet->delayed_work);
		spin_lock_bh(&bat_priv->forw_bat_list_lock);

		if (pending) {
			hlist_del(&forw_packet->list);
			batadv_forw_packet_free(forw_packet);
		}
	}
	spin_unlock_bh(&bat_priv->forw_bat_list_lock);
}
