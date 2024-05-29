int xprt_rdma_init(void)
{
	int rc;

	rc = frwr_alloc_recovery_wq();
	if (rc)
		return rc;

	rc = rpcrdma_alloc_wq();
	if (rc) {
		frwr_destroy_recovery_wq();
		return rc;
	}

	rc = xprt_register_transport(&xprt_rdma);
	if (rc) {
		rpcrdma_destroy_wq();
		frwr_destroy_recovery_wq();
		return rc;
	}

	dprintk("RPCRDMA Module Init, register RPC RDMA transport\n");

	dprintk("Defaults:\n");
	dprintk("\tSlots %d\n"
		"\tMaxInlineRead %d\n\tMaxInlineWrite %d\n",
		xprt_rdma_slot_table_entries,
		xprt_rdma_max_inline_read, xprt_rdma_max_inline_write);
	dprintk("\tPadding %d\n\tMemreg %d\n",
		xprt_rdma_inline_write_padding, xprt_rdma_memreg_strategy);

#if IS_ENABLED(CONFIG_SUNRPC_DEBUG)
	if (!sunrpc_table_header)
		sunrpc_table_header = register_sysctl_table(sunrpc_table);
#endif
	return 0;
}
