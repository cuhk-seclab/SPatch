static bool client_has_state(struct nfs4_client *clp)
{
	return client_has_openowners(clp)
#ifdef CONFIG_NFSD_PNFS
		|| !list_empty(&clp->cl_lo_states)
#endif
		|| !list_empty(&clp->cl_delegations)
		|| !list_empty(&clp->cl_sessions);
}
