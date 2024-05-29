int ptl_send_rpc(struct ptlrpc_request *request, int noreply)
{
	int rc;
	int rc2;
	int mpflag = 0;
	struct ptlrpc_connection *connection;
	lnet_handle_me_t reply_me_h;
	lnet_md_t reply_md;
	struct obd_device *obd = request->rq_import->imp_obd;

	if (OBD_FAIL_CHECK(OBD_FAIL_PTLRPC_DROP_RPC))
		return 0;

	LASSERT(request->rq_type == PTL_RPC_MSG_REQUEST);
	LASSERT(request->rq_wait_ctx == 0);

	/* If this is a re-transmit, we're required to have disengaged
	 * cleanly from the previous attempt */
	LASSERT(!request->rq_receiving_reply);
	LASSERT(!((lustre_msg_get_flags(request->rq_reqmsg) & MSG_REPLAY) &&
		(request->rq_import->imp_state == LUSTRE_IMP_FULL)));

	if (unlikely(obd != NULL && obd->obd_fail)) {
		CDEBUG(D_HA, "muting rpc for failed imp obd %s\n",
			obd->obd_name);
		/* this prevents us from waiting in ptlrpc_queue_wait */
		spin_lock(&request->rq_lock);
		request->rq_err = 1;
		spin_unlock(&request->rq_lock);
		request->rq_status = -ENODEV;
		return -ENODEV;
	}

	connection = request->rq_import->imp_connection;

	lustre_msg_set_handle(request->rq_reqmsg,
			      &request->rq_import->imp_remote_handle);
	lustre_msg_set_type(request->rq_reqmsg, PTL_RPC_MSG_REQUEST);
	lustre_msg_set_conn_cnt(request->rq_reqmsg,
				request->rq_import->imp_conn_cnt);
	lustre_msghdr_set_flags(request->rq_reqmsg,
				request->rq_import->imp_msghdr_flags);

	if (request->rq_resend)
		lustre_msg_add_flags(request->rq_reqmsg, MSG_RESENT);

	if (request->rq_memalloc)
		mpflag = cfs_memory_pressure_get_and_set();

	rc = sptlrpc_cli_wrap_request(request);
	if (rc)
		goto out;

	/* bulk register should be done after wrap_request() */
	if (request->rq_bulk != NULL) {
		rc = ptlrpc_register_bulk(request);
		if (rc != 0)
			goto out;
	}

	if (!noreply) {
		LASSERT(request->rq_replen != 0);
		if (request->rq_repbuf == NULL) {
			LASSERT(request->rq_repdata == NULL);
			LASSERT(request->rq_repmsg == NULL);
			rc = sptlrpc_cli_alloc_repbuf(request,
						      request->rq_replen);
			if (rc) {
				/* this prevents us from looping in
				 * ptlrpc_queue_wait */
				spin_lock(&request->rq_lock);
				request->rq_err = 1;
				spin_unlock(&request->rq_lock);
				request->rq_status = rc;
				goto cleanup_bulk;
			}
		} else {
			request->rq_repdata = NULL;
			request->rq_repmsg = NULL;
		}

		rc = LNetMEAttach(request->rq_reply_portal,/*XXX FIXME bug 249*/
				  connection->c_peer, request->rq_xid, 0,
				  LNET_UNLINK, LNET_INS_AFTER, &reply_me_h);
		if (rc != 0) {
			CERROR("LNetMEAttach failed: %d\n", rc);
			LASSERT(rc == -ENOMEM);
			rc = -ENOMEM;
			goto cleanup_bulk;
		}
	}

	spin_lock(&request->rq_lock);
	/* If the MD attach succeeds, there _will_ be a reply_in callback */
	request->rq_receiving_reply = !noreply;
	request->rq_req_unlink = 1;
	/* We are responsible for unlinking the reply buffer */
	request->rq_reply_unlink = !noreply;
	/* Clear any flags that may be present from previous sends. */
	request->rq_replied = 0;
	request->rq_err = 0;
	request->rq_timedout = 0;
	request->rq_net_err = 0;
	request->rq_resend = 0;
	request->rq_restart = 0;
	request->rq_reply_truncate = 0;
	spin_unlock(&request->rq_lock);

	if (!noreply) {
		reply_md.start = request->rq_repbuf;
		reply_md.length = request->rq_repbuf_len;
		/* Allow multiple early replies */
		reply_md.threshold = LNET_MD_THRESH_INF;
		/* Manage remote for early replies */
		reply_md.options = PTLRPC_MD_OPTIONS | LNET_MD_OP_PUT |
			LNET_MD_MANAGE_REMOTE |
			LNET_MD_TRUNCATE; /* allow to make EOVERFLOW error */
		reply_md.user_ptr = &request->rq_reply_cbid;
		reply_md.eq_handle = ptlrpc_eq_h;

		/* We must see the unlink callback to unset rq_reply_unlink,
		   so we can't auto-unlink */
		rc = LNetMDAttach(reply_me_h, reply_md, LNET_RETAIN,
				  &request->rq_reply_md_h);
		if (rc != 0) {
			CERROR("LNetMDAttach failed: %d\n", rc);
			LASSERT(rc == -ENOMEM);
			spin_lock(&request->rq_lock);
			/* ...but the MD attach didn't succeed... */
			request->rq_receiving_reply = 0;
			spin_unlock(&request->rq_lock);
			rc = -ENOMEM;
			goto cleanup_me;
		}

		CDEBUG(D_NET, "Setup reply buffer: %u bytes, xid %llu, portal %u\n",
		       request->rq_repbuf_len, request->rq_xid,
		       request->rq_reply_portal);
	}

	/* add references on request for request_out_callback */
	ptlrpc_request_addref(request);
	if (obd != NULL && obd->obd_svc_stats != NULL)
		lprocfs_counter_add(obd->obd_svc_stats, PTLRPC_REQACTIVE_CNTR,
			atomic_read(&request->rq_import->imp_inflight));

	OBD_FAIL_TIMEOUT(OBD_FAIL_PTLRPC_DELAY_SEND, request->rq_timeout + 5);

	do_gettimeofday(&request->rq_arrival_time);
	request->rq_sent = get_seconds();
	/* We give the server rq_timeout secs to process the req, and
	   add the network latency for our local timeout. */
	request->rq_deadline = request->rq_sent + request->rq_timeout +
		ptlrpc_at_get_net_latency(request);

	ptlrpc_pinger_sending_on_import(request->rq_import);

	DEBUG_REQ(D_INFO, request, "send flg=%x",
		  lustre_msg_get_flags(request->rq_reqmsg));
	rc = ptl_send_buf(&request->rq_req_md_h,
			  request->rq_reqbuf, request->rq_reqdata_len,
			  LNET_NOACK_REQ, &request->rq_req_cbid,
			  connection,
			  request->rq_request_portal,
			  request->rq_xid, 0);
	if (rc == 0)
		goto out;

	ptlrpc_req_finished(request);
	if (noreply)
		goto out;

 cleanup_me:
	/* MEUnlink is safe; the PUT didn't even get off the ground, and
	 * nobody apart from the PUT's target has the right nid+XID to
	 * access the reply buffer. */
	rc2 = LNetMEUnlink(reply_me_h);
	LASSERT(rc2 == 0);
	/* UNLINKED callback called synchronously */
	LASSERT(!request->rq_receiving_reply);

 cleanup_bulk:
	/* We do sync unlink here as there was no real transfer here so
	 * the chance to have long unlink to sluggish net is smaller here. */
	ptlrpc_unregister_bulk(request, 0);
 out:
	if (request->rq_memalloc)
		cfs_memory_pressure_restore(mpflag);
	return rc;
}
