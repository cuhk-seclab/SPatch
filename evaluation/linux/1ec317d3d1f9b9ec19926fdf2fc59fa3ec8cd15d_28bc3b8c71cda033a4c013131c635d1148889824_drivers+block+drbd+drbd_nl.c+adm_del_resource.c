static int adm_del_resource(struct drbd_resource *resource)
{
	struct drbd_connection *connection;

	for_each_connection(connection, resource) {
		if (connection->cstate > C_STANDALONE)
			return ERR_NET_CONFIGURED;
	}
	if (!idr_is_empty(&resource->devices))
		return ERR_RES_IN_USE;

	mutex_lock(&resources_mutex);
	list_del_rcu(&resource->resources);
	mutex_unlock(&resources_mutex);
	/* Make sure all threads have actually stopped: state handling only
	 * does drbd_thread_stop_nowait(). */
	list_for_each_entry(connection, &resource->connections, connections)
		drbd_thread_stop(&connection->worker);
	synchronize_rcu();
	drbd_free_resource(resource);
	return NO_ERROR;
}
