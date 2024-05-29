int rds_iw_cm_handle_connect(struct rdma_cm_id *cm_id,
				    struct rdma_cm_event *event)
{
	const struct rds_iw_connect_private *dp = event->param.conn.private_data;
	struct rds_iw_connect_private dp_rep;
	struct rds_connection *conn = NULL;
	struct rds_iw_connection *ic = NULL;
	struct rdma_conn_param conn_param;
	struct rds_iw_device *rds_iwdev;
	u32 version;
	int err, destroy = 1;

	/* Check whether the remote protocol version matches ours. */
	version = rds_iw_protocol_compatible(dp);
	if (!version)
		goto out;

	rdsdebug("saddr %pI4 daddr %pI4 RDSv%u.%u\n",
		 &dp->dp_saddr, &dp->dp_daddr,
		 RDS_PROTOCOL_MAJOR(version), RDS_PROTOCOL_MINOR(version));

	/* RDS/IW is not currently netns aware, thus init_net */
	conn = rds_conn_create(&init_net, dp->dp_daddr, dp->dp_saddr,
			       &rds_iw_transport, GFP_KERNEL);
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
			rds_iw_stats_inc(s_iw_listen_closed_stale);
		} else
		if (rds_conn_state(conn) == RDS_CONN_CONNECTING) {
			/* Wait and see - our connect may still be succeeding */
			rds_iw_stats_inc(s_iw_connect_raced);
		}
		mutex_unlock(&conn->c_cm_lock);
		goto out;
	}

	ic = conn->c_transport_data;

	rds_iw_set_protocol(conn, version);
	rds_iw_set_flow_control(conn, be32_to_cpu(dp->dp_credit));

	/* If the peer gave us the last packet it saw, process this as if
	 * we had received a regular ACK. */
	if (dp->dp_ack_seq)
		rds_send_drop_acked(conn, be64_to_cpu(dp->dp_ack_seq), NULL);

	BUG_ON(cm_id->context);
	BUG_ON(ic->i_cm_id);

	ic->i_cm_id = cm_id;
	cm_id->context = conn;

	rds_iwdev = ib_get_client_data(cm_id->device, &rds_iw_client);
	ic->i_dma_local_lkey = rds_iwdev->dma_local_lkey;

	/* We got halfway through setting up the ib_connection, if we
	 * fail now, we have to take the long route out of this mess. */
	destroy = 0;

	err = rds_iw_setup_qp(conn);
	if (err) {
		rds_iw_conn_error(conn, "rds_iw_setup_qp failed (%d)\n", err);
		mutex_unlock(&conn->c_cm_lock);
		goto out;
	}

	rds_iw_cm_fill_conn_param(conn, &conn_param, &dp_rep, version);

	/* rdma_accept() calls rdma_reject() internally if it fails */
	err = rdma_accept(cm_id, &conn_param);
	mutex_unlock(&conn->c_cm_lock);
	if (err) {
		rds_iw_conn_error(conn, "rdma_accept failed (%d)\n", err);
		goto out;
	}

	return 0;

out:
	rdma_reject(cm_id, NULL, 0);
	return destroy;
}
