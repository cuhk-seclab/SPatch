struct tipc_node *tipc_node_create(struct net *net, u32 addr)
{
	struct tipc_net *tn = net_generic(net, tipc_net_id);
	struct tipc_node *n_ptr, *temp_node;

	spin_lock_bh(&tn->node_list_lock);
	n_ptr = tipc_node_find(net, addr);
	if (n_ptr)
		goto exit;
	n_ptr = kzalloc(sizeof(*n_ptr), GFP_ATOMIC);
	if (!n_ptr) {
		pr_warn("Node creation failed, no memory\n");
		goto exit;
	}
	n_ptr->addr = addr;
	n_ptr->net = net;
	spin_lock_init(&n_ptr->lock);
	INIT_HLIST_NODE(&n_ptr->hash);
	INIT_LIST_HEAD(&n_ptr->list);
	INIT_LIST_HEAD(&n_ptr->publ_list);
	INIT_LIST_HEAD(&n_ptr->conn_sks);
	__skb_queue_head_init(&n_ptr->bclink.deferred_queue);
	hlist_add_head_rcu(&n_ptr->hash, &tn->node_htable[tipc_hashfn(addr)]);
	list_for_each_entry_rcu(temp_node, &tn->node_list, list) {
		if (n_ptr->addr < temp_node->addr)
			break;
	}
	list_add_tail_rcu(&n_ptr->list, &temp_node->list);
	n_ptr->action_flags = TIPC_WAIT_PEER_LINKS_DOWN;
	n_ptr->signature = INVALID_NODE_SIG;
exit:
	spin_unlock_bh(&tn->node_list_lock);
	return n_ptr;
}
