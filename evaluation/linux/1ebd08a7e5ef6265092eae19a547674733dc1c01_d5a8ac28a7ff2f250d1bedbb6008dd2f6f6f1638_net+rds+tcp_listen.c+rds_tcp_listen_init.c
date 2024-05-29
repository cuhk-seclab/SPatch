int rds_tcp_listen_init(void)
{
	struct sockaddr_in sin;
	struct socket *sock = NULL;
	int ret;

	/* MUST call sock_create_kern directly so that we avoid get_net()
	 * in sk_alloc(). Doing a get_net() will result in cleanup_net()
	 * never getting invoked, which will leave sock and other things
	 * in limbo.
	 */
	ret = sock_create_kern(current->nsproxy->net_ns, PF_INET,
			       SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret < 0)
		goto out;

	sock->sk->sk_reuse = SK_CAN_REUSE;
	rds_tcp_nonagle(sock);

	write_lock_bh(&sock->sk->sk_callback_lock);
	sock->sk->sk_user_data = sock->sk->sk_data_ready;
	sock->sk->sk_data_ready = rds_tcp_listen_data_ready;
	write_unlock_bh(&sock->sk->sk_callback_lock);

	sin.sin_family = PF_INET;
	sin.sin_addr.s_addr = (__force u32)htonl(INADDR_ANY);
	sin.sin_port = (__force u16)htons(RDS_TCP_PORT);

	ret = sock->ops->bind(sock, (struct sockaddr *)&sin, sizeof(sin));
	if (ret < 0)
		goto out;

	ret = sock->ops->listen(sock, 64);
	if (ret < 0)
		goto out;

	rds_tcp_listen_sock = sock;
	sock = NULL;
out:
	if (sock)
		sock_release(sock);
	return ret;
}
