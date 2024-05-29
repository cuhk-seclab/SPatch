int xfrm_state_walk(struct net *net, struct xfrm_state_walk *walk,
		    int (*func)(struct xfrm_state *, int, void*),
		    void *data)
{
	struct xfrm_state *state;
	struct xfrm_state_walk *x;
	int err = 0;

	if (walk->seq != 0 && list_empty(&walk->all))
		return 0;

	spin_lock_bh(&net->xfrm.xfrm_state_lock);
	if (list_empty(&walk->all))
		x = list_first_entry(&net->xfrm.state_all, struct xfrm_state_walk, all);
	else
		x = list_first_entry(&walk->all, struct xfrm_state_walk, all);
	list_for_each_entry_from(x, &net->xfrm.state_all, all) {
		if (x->state == XFRM_STATE_DEAD)
			continue;
		state = container_of(x, struct xfrm_state, km);
		if (!xfrm_id_proto_match(state->id.proto, walk->proto))
			continue;
		if (!__xfrm_state_filter_match(state, walk->filter))
			continue;
		err = func(state, walk->seq, data);
		if (err) {
			list_move_tail(&walk->all, &x->all);
			goto out;
		}
		walk->seq++;
	}
	if (walk->seq == 0) {
		err = -ENOENT;
		goto out;
	}
	list_del_init(&walk->all);
out:
	spin_unlock_bh(&net->xfrm.xfrm_state_lock);
	return err;
}
