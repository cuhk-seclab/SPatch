static bool batadv_frag_init_chain(struct batadv_frag_table_entry *chain,
				   u16 seqno)
{
	lockdep_assert_held(&chain->lock);

	if (chain->seqno == seqno)
		return false;

	if (!hlist_empty(&chain->head))
		batadv_frag_clear_chain(&chain->head);

	chain->size = 0;
	chain->seqno = seqno;

	return true;
}
