static void disconnect_all_peers(void)
{
	struct lowpan_dev *entry;
	struct lowpan_peer *peer, *tmp_peer, *new_peer;
	struct list_head peers;

	INIT_LIST_HEAD(&peers);

	/* We make a separate list of peers as the close_cb() will
	 * modify the device peers list so it is better not to mess
	 * with the same list at the same time.
	 */

	rcu_read_lock();

	list_for_each_entry_rcu(entry, &bt_6lowpan_devices, list) {
		list_for_each_entry_rcu(peer, &entry->peers, list) {
			new_peer = kmalloc(sizeof(*new_peer), GFP_ATOMIC);
			if (!new_peer)
				break;

			new_peer->chan = peer->chan;
			INIT_LIST_HEAD(&new_peer->list);

			list_add(&new_peer->list, &peers);
		}
	}

	rcu_read_unlock();

	spin_lock(&devices_lock);
	list_for_each_entry_safe(peer, tmp_peer, &peers, list) {
		l2cap_chan_close(peer->chan, ENOENT);

		list_del_rcu(&peer->list);
		kfree_rcu(peer, rcu);
	}
	spin_unlock(&devices_lock);
}
