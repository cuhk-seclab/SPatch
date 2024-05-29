static int rds_tcp_laddr_check(struct net *net, __be32 addr)
{
	if (inet_addr_type(net, addr) == RTN_LOCAL)
		return 0;
	return -EADDRNOTAVAIL;
}
