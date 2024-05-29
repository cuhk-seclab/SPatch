int rds_tcp_conn_connect(struct rds_connection *conn)
{
	struct socket *sock = NULL;
	struct sockaddr_in src, dest;
	int ret;

	ret = sock_create_kern(rds_conn_net(conn), PF_INET,
			       SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret < 0)
		goto out;

	rds_tcp_tune(sock);

	src.sin_family = AF_INET;
	src.sin_addr.s_addr = (__force u32)conn->c_laddr;
	src.sin_port = (__force u16)htons(0);

	ret = sock->ops->bind(sock, (struct sockaddr *)&src, sizeof(src));
	if (ret) {
		rdsdebug("bind failed with %d at address %pI4\n",
			 ret, &conn->c_laddr);
		goto out;
	}

	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = (__force u32)conn->c_faddr;
	dest.sin_port = (__force u16)htons(RDS_TCP_PORT);

	/*
	 * once we call connect() we can start getting callbacks and they
	 * own the socket
	 */
	rds_tcp_set_callbacks(sock, conn);
	ret = sock->ops->connect(sock, (struct sockaddr *)&dest, sizeof(dest),
				 O_NONBLOCK);

	rdsdebug("connect to address %pI4 returned %d\n", &conn->c_faddr, ret);
	if (ret == -EINPROGRESS)
		ret = 0;
	if (ret == 0)
		sock = NULL;
	else
		rds_tcp_restore_callbacks(sock, conn->c_transport_data);

out:
	if (sock)
		sock_release(sock);
	return ret;
}
