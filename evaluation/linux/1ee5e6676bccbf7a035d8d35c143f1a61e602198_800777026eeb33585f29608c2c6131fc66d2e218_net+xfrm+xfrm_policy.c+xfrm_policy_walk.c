int xfrm_policy_walk(struct net *net, struct xfrm_policy_walk *walk,
		     int (*func)(struct xfrm_policy *, int, int, void*),
		     void *data)
{
	struct xfrm_policy *pol;
	struct xfrm_policy_walk_entry *x;
	int error = 0;

	if (walk->type >= XFRM_POLICY_TYPE_MAX &&
	    walk->type != XFRM_POLICY_TYPE_ANY)
		return -EINVAL;

	if (list_empty(&walk->walk.all) && walk->seq != 0)
		return 0;

	write_lock_bh(&net->xfrm.xfrm_policy_lock);
	if (list_empty(&walk->walk.all))
		x = list_first_entry(&net->xfrm.policy_all, struct xfrm_policy_walk_entry, all);
	else
		x = list_first_entry(&walk->walk.all,
				     struct xfrm_policy_walk_entry, all);

	list_for_each_entry_from(x, &net->xfrm.policy_all, all) {
		if (x->dead)
			continue;
		pol = container_of(x, struct xfrm_policy, walk);
		if (walk->type != XFRM_POLICY_TYPE_ANY &&
		    walk->type != pol->type)
			continue;
		error = func(pol, xfrm_policy_id2dir(pol->index),
			     walk->seq, data);
		if (error) {
			list_move_tail(&walk->walk.all, &x->all);
			goto out;
		}
		walk->seq++;
	}
	if (walk->seq == 0) {
		error = -ENOENT;
		goto out;
	}
	list_del_init(&walk->walk.all);
out:
	write_unlock_bh(&net->xfrm.xfrm_policy_lock);
	return error;
}
