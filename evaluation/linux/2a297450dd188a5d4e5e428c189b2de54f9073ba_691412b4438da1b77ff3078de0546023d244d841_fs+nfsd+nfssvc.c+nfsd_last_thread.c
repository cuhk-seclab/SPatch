static void nfsd_last_thread(struct svc_serv *serv, struct net *net)
{
	struct nfsd_net *nn = net_generic(net, nfsd_net_id);

	unregister_inetaddr_notifier(&nfsd_inetaddr_notifier);
#if IS_ENABLED(CONFIG_IPV6)
	unregister_inet6addr_notifier(&nfsd_inet6addr_notifier);
#endif
	/*
	 * write_ports can create the server without actually starting
	 * any threads--if we get shut down before any threads are
	 * started, then nfsd_last_thread will be run before any of this
	 * other initialization has been done except the rpcb information.
	 */
	svc_rpcb_cleanup(serv, net);
	if (!nn->nfsd_net_up)
		return;

	nfsd_shutdown_net(net);
	printk(KERN_WARNING "nfsd: last server has exited, flushing export "
			    "cache\n");
	nfsd_export_flush(net);
}
