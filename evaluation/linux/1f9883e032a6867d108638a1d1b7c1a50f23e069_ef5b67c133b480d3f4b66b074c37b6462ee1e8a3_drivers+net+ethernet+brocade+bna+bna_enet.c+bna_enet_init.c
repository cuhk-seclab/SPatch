static void
bna_enet_init(struct bna_enet *enet, struct bna *bna)
{
	enet->bna = bna;
	enet->flags = 0;
	enet->mtu = 0;
	enet->type = BNA_ENET_T_REGULAR;

	enet->stop_cbfn = NULL;
	enet->stop_cbarg = NULL;

	enet->mtu_cbfn = NULL;

	bfa_fsm_set_state(enet, bna_enet_sm_stopped);
}
