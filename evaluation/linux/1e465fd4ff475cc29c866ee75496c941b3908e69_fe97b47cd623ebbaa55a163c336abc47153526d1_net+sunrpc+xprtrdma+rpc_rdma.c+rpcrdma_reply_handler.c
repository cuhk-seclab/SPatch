void
rpcrdma_reply_handler(struct rpcrdma_rep *rep)
{
	struct rpcrdma_msg *headerp;
	struct rpcrdma_req *req;
	struct rpc_rqst *rqst;
	struct rpcrdma_xprt *r_xprt = rep->rr_rxprt;
	struct rpc_xprt *xprt = &r_xprt->rx_xprt;
	__be32 *iptr;
	int rdmalen, status;
	unsigned long cwnd;
	u32 credits;

	dprintk("RPC:       %s: incoming rep %p\n", __func__, rep);

	if (rep->rr_len == RPCRDMA_BAD_LEN)
		goto out_badstatus;
	if (rep->rr_len < RPCRDMA_HDRLEN_MIN)
		goto out_shortreply;

	headerp = rdmab_to_msg(rep->rr_rdmabuf);
	if (headerp->rm_vers != rpcrdma_version)
		goto out_badversion;

	/* Match incoming rpcrdma_rep to an rpcrdma_req to
	 * get context for handling any incoming chunks.
	 */
	spin_lock_bh(&xprt->transport_lock);
	rqst = xprt_lookup_rqst(xprt, headerp->rm_xid);
	if (!rqst)
		goto out_nomatch;

	req = rpcr_to_rdmar(rqst);
	if (req->rl_reply)
		goto out_duplicate;

	dprintk("RPC:       %s: reply 0x%p completes request 0x%p\n"
		"                   RPC request 0x%p xid 0x%08x\n",
			__func__, rep, req, rqst,
			be32_to_cpu(headerp->rm_xid));

	/* from here on, the reply is no longer an orphan */
	req->rl_reply = rep;
	xprt->reestablish_timeout = 0;

	/* check for expected message types */
	/* The order of some of these tests is important. */
	switch (headerp->rm_type) {
	case rdma_msg:
		/* never expect read chunks */
		/* never expect reply chunks (two ways to check) */
		/* never expect write chunks without having offered RDMA */
		if (headerp->rm_body.rm_chunks[0] != xdr_zero ||
		    (headerp->rm_body.rm_chunks[1] == xdr_zero &&
		     headerp->rm_body.rm_chunks[2] != xdr_zero) ||
		    (headerp->rm_body.rm_chunks[1] != xdr_zero &&
		     req->rl_nchunks == 0))
			goto badheader;
		if (headerp->rm_body.rm_chunks[1] != xdr_zero) {
			/* count any expected write chunks in read reply */
			/* start at write chunk array count */
			iptr = &headerp->rm_body.rm_chunks[2];
			rdmalen = rpcrdma_count_chunks(rep,
						req->rl_nchunks, 1, &iptr);
			/* check for validity, and no reply chunk after */
			if (rdmalen < 0 || *iptr++ != xdr_zero)
				goto badheader;
			rep->rr_len -=
			    ((unsigned char *)iptr - (unsigned char *)headerp);
			status = rep->rr_len + rdmalen;
			r_xprt->rx_stats.total_rdma_reply += rdmalen;
			/* special case - last chunk may omit padding */
			if (rdmalen &= 3) {
				rdmalen = 4 - rdmalen;
				status += rdmalen;
			}
		} else {
			/* else ordinary inline */
			rdmalen = 0;
			iptr = (__be32 *)((unsigned char *)headerp +
							RPCRDMA_HDRLEN_MIN);
			rep->rr_len -= RPCRDMA_HDRLEN_MIN;
			status = rep->rr_len;
		}
		/* Fix up the rpc results for upper layer */
		rpcrdma_inline_fixup(rqst, (char *)iptr, rep->rr_len, rdmalen);
		break;

	case rdma_nomsg:
		/* never expect read or write chunks, always reply chunks */
		if (headerp->rm_body.rm_chunks[0] != xdr_zero ||
		    headerp->rm_body.rm_chunks[1] != xdr_zero ||
		    headerp->rm_body.rm_chunks[2] != xdr_one ||
		    req->rl_nchunks == 0)
			goto badheader;
		iptr = (__be32 *)((unsigned char *)headerp +
							RPCRDMA_HDRLEN_MIN);
		rdmalen = rpcrdma_count_chunks(rep, req->rl_nchunks, 0, &iptr);
		if (rdmalen < 0)
			goto badheader;
		r_xprt->rx_stats.total_rdma_reply += rdmalen;
		/* Reply chunk buffer already is the reply vector - no fixup. */
		status = rdmalen;
		break;

badheader:
	default:
		dprintk("%s: invalid rpcrdma reply header (type %d):"
				" chunks[012] == %d %d %d"
				" expected chunks <= %d\n",
				__func__, be32_to_cpu(headerp->rm_type),
				headerp->rm_body.rm_chunks[0],
				headerp->rm_body.rm_chunks[1],
				headerp->rm_body.rm_chunks[2],
				req->rl_nchunks);
		status = -EIO;
		r_xprt->rx_stats.bad_reply_count++;
		break;
	}

	credits = be32_to_cpu(headerp->rm_credit);
	if (credits == 0)
		credits = 1;	/* don't deadlock */
	else if (credits > r_xprt->rx_buf.rb_max_requests)
		credits = r_xprt->rx_buf.rb_max_requests;

	cwnd = xprt->cwnd;
	xprt->cwnd = credits << RPC_CWNDSHIFT;
	if (xprt->cwnd > cwnd)
		xprt_release_rqst_cong(rqst->rq_task);

	xprt_complete_rqst(rqst->rq_task, status);
	spin_unlock_bh(&xprt->transport_lock);
	dprintk("RPC:       %s: xprt_complete_rqst(0x%p, 0x%p, %d)\n",
			__func__, xprt, rqst, status);
	return;

out_badstatus:
	rpcrdma_recv_buffer_put(rep);
	if (r_xprt->rx_ep.rep_connected == 1) {
		r_xprt->rx_ep.rep_connected = -EIO;
		rpcrdma_conn_func(&r_xprt->rx_ep);
	}
	return;

out_shortreply:
	dprintk("RPC:       %s: short/invalid reply\n", __func__);
	goto repost;

out_badversion:
	dprintk("RPC:       %s: invalid version %d\n",
		__func__, be32_to_cpu(headerp->rm_vers));
	goto repost;

out_nomatch:
	spin_unlock_bh(&xprt->transport_lock);
	dprintk("RPC:       %s: no match for incoming xid 0x%08x len %d\n",
		__func__, be32_to_cpu(headerp->rm_xid),
		rep->rr_len);
	goto repost;

out_duplicate:
	spin_unlock_bh(&xprt->transport_lock);
	dprintk("RPC:       %s: "
		"duplicate reply %p to RPC request %p: xid 0x%08x\n",
		__func__, rep, req, be32_to_cpu(headerp->rm_xid));

repost:
	r_xprt->rx_stats.bad_reply_count++;
	if (rpcrdma_ep_post_recv(&r_xprt->rx_ia, &r_xprt->rx_ep, rep))
		rpcrdma_recv_buffer_put(rep);
}
