int iscsit_close_connection(
	struct iscsi_conn *conn)
{
	int conn_logout = (conn->conn_state == TARG_CONN_STATE_IN_LOGOUT);
	struct iscsi_session	*sess = conn->sess;

	pr_debug("Closing iSCSI connection CID %hu on SID:"
		" %u\n", conn->cid, sess->sid);
	/*
	 * Always up conn_logout_comp for the traditional TCP case just in case
	 * the RX Thread in iscsi_target_rx_opcode() is sleeping and the logout
	 * response never got sent because the connection failed.
	 *
	 * However for iser-target, isert_wait4logout() is using conn_logout_comp
	 * to signal logout response TX interrupt completion.  Go ahead and skip
	 * this for iser since isert_rx_opcode() does not wait on logout failure,
	 * and to avoid iscsi_conn pointer dereference in iser-target code.
	 */
	if (conn->conn_transport->transport_type == ISCSI_TCP)
		complete(&conn->conn_logout_comp);

	iscsi_release_thread_set(conn);

	iscsit_stop_timers_for_cmds(conn);
	iscsit_stop_nopin_response_timer(conn);
	iscsit_stop_nopin_timer(conn);

	if (conn->conn_transport->iscsit_wait_conn)
		conn->conn_transport->iscsit_wait_conn(conn);

	/*
	 * During Connection recovery drop unacknowledged out of order
	 * commands for this connection, and prepare the other commands
	 * for realligence.
	 *
	 * During normal operation clear the out of order commands (but
	 * do not free the struct iscsi_ooo_cmdsn's) and release all
	 * struct iscsi_cmds.
	 */
	if (atomic_read(&conn->connection_recovery)) {
		iscsit_discard_unacknowledged_ooo_cmdsns_for_conn(conn);
		iscsit_prepare_cmds_for_realligance(conn);
	} else {
		iscsit_clear_ooo_cmdsns_for_conn(conn);
		iscsit_release_commands_from_conn(conn);
	}
	iscsit_free_queue_reqs_for_conn(conn);

	/*
	 * Handle decrementing session or connection usage count if
	 * a logout response was not able to be sent because the
	 * connection failed.  Fall back to Session Recovery here.
	 */
	if (atomic_read(&conn->conn_logout_remove)) {
		if (conn->conn_logout_reason == ISCSI_LOGOUT_REASON_CLOSE_SESSION) {
			iscsit_dec_conn_usage_count(conn);
			iscsit_dec_session_usage_count(sess);
		}
		if (conn->conn_logout_reason == ISCSI_LOGOUT_REASON_CLOSE_CONNECTION)
			iscsit_dec_conn_usage_count(conn);

		atomic_set(&conn->conn_logout_remove, 0);
		atomic_set(&sess->session_reinstatement, 0);
		atomic_set(&sess->session_fall_back_to_erl0, 1);
	}

	spin_lock_bh(&sess->conn_lock);
	list_del(&conn->conn_list);

	/*
	 * Attempt to let the Initiator know this connection failed by
	 * sending an Connection Dropped Async Message on another
	 * active connection.
	 */
	if (atomic_read(&conn->connection_recovery))
		iscsit_build_conn_drop_async_message(conn);

	spin_unlock_bh(&sess->conn_lock);

	/*
	 * If connection reinstatement is being performed on this connection,
	 * up the connection reinstatement semaphore that is being blocked on
	 * in iscsit_cause_connection_reinstatement().
	 */
	spin_lock_bh(&conn->state_lock);
	if (atomic_read(&conn->sleep_on_conn_wait_comp)) {
		spin_unlock_bh(&conn->state_lock);
		complete(&conn->conn_wait_comp);
		wait_for_completion(&conn->conn_post_wait_comp);
		spin_lock_bh(&conn->state_lock);
	}

	/*
	 * If connection reinstatement is being performed on this connection
	 * by receiving a REMOVECONNFORRECOVERY logout request, up the
	 * connection wait rcfr semaphore that is being blocked on
	 * an iscsit_connection_reinstatement_rcfr().
	 */
	if (atomic_read(&conn->connection_wait_rcfr)) {
		spin_unlock_bh(&conn->state_lock);
		complete(&conn->conn_wait_rcfr_comp);
		wait_for_completion(&conn->conn_post_wait_comp);
		spin_lock_bh(&conn->state_lock);
	}
	atomic_set(&conn->connection_reinstatement, 1);
	spin_unlock_bh(&conn->state_lock);

	/*
	 * If any other processes are accessing this connection pointer we
	 * must wait until they have completed.
	 */
	iscsit_check_conn_usage_count(conn);

	if (conn->conn_rx_hash.tfm)
		crypto_free_hash(conn->conn_rx_hash.tfm);
	if (conn->conn_tx_hash.tfm)
		crypto_free_hash(conn->conn_tx_hash.tfm);

	free_cpumask_var(conn->conn_cpumask);

	kfree(conn->conn_ops);
	conn->conn_ops = NULL;

	if (conn->sock)
		sock_release(conn->sock);

	if (conn->conn_transport->iscsit_free_conn)
		conn->conn_transport->iscsit_free_conn(conn);

	iscsit_put_transport(conn->conn_transport);

	conn->thread_set = NULL;

	pr_debug("Moving to TARG_CONN_STATE_FREE.\n");
	conn->conn_state = TARG_CONN_STATE_FREE;
	kfree(conn);

	spin_lock_bh(&sess->conn_lock);
	atomic_dec(&sess->nconn);
	pr_debug("Decremented iSCSI connection count to %hu from node:"
		" %s\n", atomic_read(&sess->nconn),
		sess->sess_ops->InitiatorName);
	/*
	 * Make sure that if one connection fails in an non ERL=2 iSCSI
	 * Session that they all fail.
	 */
	if ((sess->sess_ops->ErrorRecoveryLevel != 2) && !conn_logout &&
	     !atomic_read(&sess->session_logout))
		atomic_set(&sess->session_fall_back_to_erl0, 1);

	/*
	 * If this was not the last connection in the session, and we are
	 * performing session reinstatement or falling back to ERL=0, call
	 * iscsit_stop_session() without sleeping to shutdown the other
	 * active connections.
	 */
	if (atomic_read(&sess->nconn)) {
		if (!atomic_read(&sess->session_reinstatement) &&
		    !atomic_read(&sess->session_fall_back_to_erl0)) {
			spin_unlock_bh(&sess->conn_lock);
			return 0;
		}
		if (!atomic_read(&sess->session_stop_active)) {
			atomic_set(&sess->session_stop_active, 1);
			spin_unlock_bh(&sess->conn_lock);
			iscsit_stop_session(sess, 0, 0);
			return 0;
		}
		spin_unlock_bh(&sess->conn_lock);
		return 0;
	}

	/*
	 * If this was the last connection in the session and one of the
	 * following is occurring:
	 *
	 * Session Reinstatement is not being performed, and are falling back
	 * to ERL=0 call iscsit_close_session().
	 *
	 * Session Logout was requested.  iscsit_close_session() will be called
	 * elsewhere.
	 *
	 * Session Continuation is not being performed, start the Time2Retain
	 * handler and check if sleep_on_sess_wait_sem is active.
	 */
	if (!atomic_read(&sess->session_reinstatement) &&
	     atomic_read(&sess->session_fall_back_to_erl0)) {
		spin_unlock_bh(&sess->conn_lock);
		target_put_session(sess->se_sess);

		return 0;
	} else if (atomic_read(&sess->session_logout)) {
		pr_debug("Moving to TARG_SESS_STATE_FREE.\n");
		sess->session_state = TARG_SESS_STATE_FREE;
		spin_unlock_bh(&sess->conn_lock);

		if (atomic_read(&sess->sleep_on_sess_wait_comp))
			complete(&sess->session_wait_comp);

		return 0;
	} else {
		pr_debug("Moving to TARG_SESS_STATE_FAILED.\n");
		sess->session_state = TARG_SESS_STATE_FAILED;

		if (!atomic_read(&sess->session_continuation)) {
			spin_unlock_bh(&sess->conn_lock);
			iscsit_start_time2retain_handler(sess);
		} else
			spin_unlock_bh(&sess->conn_lock);

		if (atomic_read(&sess->sleep_on_sess_wait_comp))
			complete(&sess->session_wait_comp);

		return 0;
	}
	spin_unlock_bh(&sess->conn_lock);

	return 0;
}
