void
bna_bfi_bw_update_aen(struct bna_tx_mod *tx_mod)
{
	struct bna_tx *tx;

	list_for_each_entry(tx, &tx_mod->tx_active_q, qe)
		bfa_fsm_send_event(tx, TX_E_BW_UPDATE);
}
