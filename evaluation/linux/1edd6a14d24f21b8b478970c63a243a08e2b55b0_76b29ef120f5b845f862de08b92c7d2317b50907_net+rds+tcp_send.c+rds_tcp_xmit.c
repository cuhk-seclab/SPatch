int rds_tcp_xmit(struct rds_connection *conn, struct rds_message *rm,
	         unsigned int hdr_off, unsigned int sg, unsigned int off)
{
	struct rds_tcp_connection *tc = conn->c_transport_data;
	int done = 0;
	int ret = 0;

	if (hdr_off == 0) {
		/*
		 * m_ack_seq is set to the sequence number of the last byte of
		 * header and data.  see rds_tcp_is_acked().
		 */
		tc->t_last_sent_nxt = rds_tcp_snd_nxt(tc);
		rm->m_ack_seq = tc->t_last_sent_nxt +
				sizeof(struct rds_header) +
				be32_to_cpu(rm->m_inc.i_hdr.h_len) - 1;
		smp_mb__before_atomic();
		set_bit(RDS_MSG_HAS_ACK_SEQ, &rm->m_flags);
		tc->t_last_expected_una = rm->m_ack_seq + 1;

		rdsdebug("rm %p tcp nxt %u ack_seq %llu\n",
			 rm, rds_tcp_snd_nxt(tc),
			 (unsigned long long)rm->m_ack_seq);
	}

	if (hdr_off < sizeof(struct rds_header)) {
		/* see rds_tcp_write_space() */
		set_bit(SOCK_NOSPACE, &tc->t_sock->sk->sk_socket->flags);

		ret = rds_tcp_sendmsg(tc->t_sock,
				      (void *)&rm->m_inc.i_hdr + hdr_off,
				      sizeof(rm->m_inc.i_hdr) - hdr_off);
		if (ret < 0)
			goto out;
		done += ret;
		if (hdr_off + done != sizeof(struct rds_header))
			goto out;
	}

	while (sg < rm->data.op_nents) {
		ret = tc->t_sock->ops->sendpage(tc->t_sock,
						sg_page(&rm->data.op_sg[sg]),
						rm->data.op_sg[sg].offset + off,
						rm->data.op_sg[sg].length - off,
						MSG_DONTWAIT|MSG_NOSIGNAL);
		rdsdebug("tcp sendpage %p:%u:%u ret %d\n", (void *)sg_page(&rm->data.op_sg[sg]),
			 rm->data.op_sg[sg].offset + off, rm->data.op_sg[sg].length - off,
			 ret);
		if (ret <= 0)
			break;

		off += ret;
		done += ret;
		if (off == rm->data.op_sg[sg].length) {
			off = 0;
			sg++;
		}
	}

out:
	if (ret <= 0) {
		/* write_space will hit after EAGAIN, all else fatal */
		if (ret == -EAGAIN) {
			rds_tcp_stats_inc(s_tcp_sndbuf_full);
			ret = 0;
		} else {
			printk(KERN_WARNING "RDS/tcp: send to %pI4 "
			       "returned %d, disconnecting and reconnecting\n",
			       &conn->c_faddr, ret);
			rds_conn_drop(conn);
		}
	}
	if (done == 0)
		done = ret;
	return done;
}
