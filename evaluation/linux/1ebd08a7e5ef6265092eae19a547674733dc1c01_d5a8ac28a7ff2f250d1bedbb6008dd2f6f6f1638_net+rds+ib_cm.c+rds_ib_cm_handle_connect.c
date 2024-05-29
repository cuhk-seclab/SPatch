int rds_ib_cm_handle_connect(struct rdma_cm_id *cm_id,
				    struct rdma_cm_event *event)
{
	__be64 lguid = cm_id->route.path_rec->sgid.global.interface_id;
	__be64 fguid = cm_id->route.path_rec->dgid.global.interface_id;
	const struct rds_ib_connect_private *dp = event->param.conn.private_data;
	struct rds_ib_connect_private dp_rep;
	struct rds_connection *conn = NULL;
	struct rds_ib_connection *ic = NULL;
	struct rdma_conn_param conn_param;
	u32 version;
	int err = 1, destroy = 1;

	/* Check whether the remote protocol version matches ours. */
	version = rds_ib_protocol_compatible(event);
	if (!version)
		goto out;

	rdsdebug("saddr %pI4 daddr %pI4 RDSv%u.%u lguid 0x%llx fguid "
		 "0x%llx\n", &dp->dp_saddr, &dp->dp_daddr,
		 RDS_PROTOCOL_MAJOR(version), RDS_PROTOCOL_MINOR(version),
		 (unsigned long long)be64_to_cpu(lguid),
		 (unsigned long long)be64_to_cpu(fguid));

	/* RDS/IB is not currently netns aware, thus init_net */
	conn = rds_conn_create(&init_net, dp->dp_daddr, dp->dp_saddr,
			       &rds_ib_transport, GFP_KERNEL);
	if (IS_ERR(conn)) {
		rdsdebug("rds_conn_create failed (%ld)\n", PTR_ERR(conn));
		conn = NULL;
		goto out;
	}

	/*
	 * The connection request may occur while the
	 * previous connection exist, e.g. in case of failover.
	 * But as connections may be initiated simultaneously
	 * by both hosts, we have a random backoff mechanism -
	 * see the comment above rds_queue_reconnect()
	 */
	mutex_lock(&conn->c_cm_lock);
	if (!rds_conn_transition(conn, RDS_CONN_DOWN, RDS_CONN_CONNECTING)) {
		if (rds_conn_state(conn) == RDS_CONN_UP) {
			rdsdebug("incoming connect while connecting\n");
			rds_conn_drop(conn);
			rds_ib_stats_inc(s_ib_listen_closed_stale);
		} else
		if (rds_conn_state(conn) == RDS_CONN_CONNECTING) {
			/* Wait and see - our connect may still be succeeding */
			rds_ib_stats_inc(s_ib_connect_raced);
		}
		goto out;
	}

	ic = conn->c_transport_data;

	rds_ib_set_protocol(conn, version);
	rds_ib_set_flow_control(conn, be32_to_cpu(dp->dp_credit));

	/* If the peer gave us the last packet it saw, process this as if
	 * we had received a regular ACK. */
	if (dp->dp_ack_seq)
		rds_send_drop_acked(conn, be64_to_cpu(dp->dp_ack_seq), NULL);

	BUG_ON(cm_id->context);
	BUG_ON(ic->i_cm_id);

	ic->i_cm_id = cm_id;
	cm_id->context = conn;

	/* We got halfway through setting up the ib_connection, if we
	 * fail now, we have to take the long route out of this mess. */
	destroy = 0;

	err = rds_ib_setup_qp(conn);
	if (err) {
		rds_ib_conn_error(conn, "rds_ib_setup_qp failed (%d)\n", err);
		goto out;
	}

	rds_ib_cm_fill_conn_param(conn, &conn_param, &dp_rep, version,
		event->param.conn.responder_resources,
		event->param.conn.initiator_depth);

	/* rdma_accept() calls rdma_reject() internally if it fails */
	err = rdma_accept(cm_id, &conn_param);
	if (err)
		rds_ib_conn_error(conn, "rdma_accept failed (%d)\n", err);

out:
	if (conn)
		mutex_unlock(&conn->c_cm_lock);
	if (err)
		rdma_reject(cm_id, NULL, 0);
	return destroy;
}
