static int l2cap_le_connect_req(struct l2cap_conn *conn,
				struct l2cap_cmd_hdr *cmd, u16 cmd_len,
				u8 *data)
{
	struct l2cap_le_conn_req *req = (struct l2cap_le_conn_req *) data;
	struct l2cap_le_conn_rsp rsp;
	struct l2cap_chan *chan, *pchan;
	u16 dcid, scid, credits, mtu, mps;
	__le16 psm;
	u8 result;

	if (cmd_len != sizeof(*req))
		return -EPROTO;

	scid = __le16_to_cpu(req->scid);
	mtu  = __le16_to_cpu(req->mtu);
	mps  = __le16_to_cpu(req->mps);
	psm  = req->psm;
	dcid = 0;
	credits = 0;

	if (mtu < 23 || mps < 23)
		return -EPROTO;

	BT_DBG("psm 0x%2.2x scid 0x%4.4x mtu %u mps %u", __le16_to_cpu(psm),
	       scid, mtu, mps);

	/* Check if we have socket listening on psm */
	pchan = l2cap_global_chan_by_psm(BT_LISTEN, psm, &conn->hcon->src,
					 &conn->hcon->dst, LE_LINK);
	if (!pchan) {
		result = L2CAP_CR_BAD_PSM;
		chan = NULL;
		goto response;
	}

	mutex_lock(&conn->chan_lock);
	l2cap_chan_lock(pchan);

	if (!smp_sufficient_security(conn->hcon, pchan->sec_level,
				     SMP_ALLOW_STK)) {
		result = L2CAP_CR_AUTHENTICATION;
		chan = NULL;
		goto response_unlock;
	}

	/* Check for valid dynamic CID range */
	if (scid < L2CAP_CID_DYN_START || scid > L2CAP_CID_LE_DYN_END) {
		result = L2CAP_CR_INVALID_SCID;
		chan = NULL;
		goto response_unlock;
	}

	/* Check if we already have channel with that dcid */
	if (__l2cap_get_chan_by_dcid(conn, scid)) {
		result = L2CAP_CR_SCID_IN_USE;
		chan = NULL;
		goto response_unlock;
	}

	chan = pchan->ops->new_connection(pchan);
	if (!chan) {
		result = L2CAP_CR_NO_MEM;
		goto response_unlock;
	}

	l2cap_le_flowctl_init(chan);

	bacpy(&chan->src, &conn->hcon->src);
	bacpy(&chan->dst, &conn->hcon->dst);
	chan->src_type = bdaddr_src_type(conn->hcon);
	chan->dst_type = bdaddr_dst_type(conn->hcon);
	chan->psm  = psm;
	chan->dcid = scid;
	chan->omtu = mtu;
	chan->remote_mps = mps;
	chan->tx_credits = __le16_to_cpu(req->credits);

	__l2cap_chan_add(conn, chan);
	dcid = chan->scid;
	credits = chan->rx_credits;

	__set_chan_timer(chan, chan->ops->get_sndtimeo(chan));

	chan->ident = cmd->ident;

	if (test_bit(FLAG_DEFER_SETUP, &chan->flags)) {
		l2cap_state_change(chan, BT_CONNECT2);
		/* The following result value is actually not defined
		 * for LE CoC but we use it to let the function know
		 * that it should bail out after doing its cleanup
		 * instead of sending a response.
		 */
		result = L2CAP_CR_PEND;
		chan->ops->defer(chan);
	} else {
		l2cap_chan_ready(chan);
		result = L2CAP_CR_SUCCESS;
	}

response_unlock:
	l2cap_chan_unlock(pchan);
	mutex_unlock(&conn->chan_lock);
	l2cap_chan_put(pchan);

	if (result == L2CAP_CR_PEND)
		return 0;

response:
	if (chan) {
		rsp.mtu = cpu_to_le16(chan->imtu);
		rsp.mps = cpu_to_le16(chan->mps);
	} else {
		rsp.mtu = 0;
		rsp.mps = 0;
	}

	rsp.dcid    = cpu_to_le16(dcid);
	rsp.credits = cpu_to_le16(credits);
	rsp.result  = cpu_to_le16(result);

	l2cap_send_cmd(conn, cmd->ident, L2CAP_LE_CONN_RSP, sizeof(rsp), &rsp);

	return 0;
}
