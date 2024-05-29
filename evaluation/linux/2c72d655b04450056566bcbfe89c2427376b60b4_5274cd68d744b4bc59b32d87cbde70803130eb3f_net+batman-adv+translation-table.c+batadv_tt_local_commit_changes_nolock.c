static void batadv_tt_local_commit_changes_nolock(struct batadv_priv *bat_priv)
{
	lockdep_assert_held(&bat_priv->tt.commit_lock);

	/* Update multicast addresses in local translation table */
	batadv_mcast_mla_update(bat_priv);

	if (atomic_read(&bat_priv->tt.local_changes) < 1) {
		if (!batadv_atomic_dec_not_zero(&bat_priv->tt.ogm_append_cnt))
			batadv_tt_tvlv_container_update(bat_priv);
		return;
	}

	batadv_tt_local_set_flags(bat_priv, BATADV_TT_CLIENT_NEW, false, true);

	batadv_tt_local_purge_pending_clients(bat_priv);
	batadv_tt_local_update_crc(bat_priv);

	/* Increment the TTVN only once per OGM interval */
	atomic_inc(&bat_priv->tt.vn);
	batadv_dbg(BATADV_DBG_TT, bat_priv,
		   "Local changes committed, updating to ttvn %u\n",
		   (u8)atomic_read(&bat_priv->tt.vn));

	/* reset the sending counter */
	atomic_set(&bat_priv->tt.ogm_append_cnt, BATADV_TT_OGM_APPEND_MAX);
	batadv_tt_tvlv_container_update(bat_priv);
}
