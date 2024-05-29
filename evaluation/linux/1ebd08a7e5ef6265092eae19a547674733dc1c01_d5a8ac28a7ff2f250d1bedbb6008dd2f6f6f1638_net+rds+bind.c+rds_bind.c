int rds_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
{
	struct sock *sk = sock->sk;
	struct sockaddr_in *sin = (struct sockaddr_in *)uaddr;
	struct rds_sock *rs = rds_sk_to_rs(sk);
	struct rds_transport *trans;
	int ret = 0;

	lock_sock(sk);

	if (addr_len != sizeof(struct sockaddr_in) ||
	    sin->sin_family != AF_INET ||
	    rs->rs_bound_addr ||
	    sin->sin_addr.s_addr == htonl(INADDR_ANY)) {
		ret = -EINVAL;
		goto out;
	}

	ret = rds_add_bound(rs, sin->sin_addr.s_addr, &sin->sin_port);
	if (ret)
		goto out;

	if (rs->rs_transport) { /* previously bound */
		ret = 0;
		goto out;
	}
	trans = rds_trans_get_preferred(sock_net(sock->sk),
					sin->sin_addr.s_addr);
	if (!trans) {
		ret = -EADDRNOTAVAIL;
		rds_remove_bound(rs);
		printk_ratelimited(KERN_INFO "RDS: rds_bind() could not find a transport, "
				"load rds_tcp or rds_rdma?\n");
		goto out;
	}

	rs->rs_transport = trans;
	ret = 0;

out:
	release_sock(sk);

	/* we might have called rds_remove_bound on error */
	if (ret)
		synchronize_rcu();
	return ret;
}
