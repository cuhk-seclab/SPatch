int btrfs_add_delayed_tree_ref(struct btrfs_fs_info *fs_info,
			       struct btrfs_trans_handle *trans,
			       u64 bytenr, u64 num_bytes, u64 parent,
			       u64 ref_root,  int level, int action,
			       struct btrfs_delayed_extent_op *extent_op)
{
	struct btrfs_delayed_tree_ref *ref;
	struct btrfs_delayed_ref_head *head_ref;
	struct btrfs_delayed_ref_root *delayed_refs;
	struct btrfs_qgroup_extent_record *record = NULL;

	BUG_ON(extent_op && extent_op->is_data);
	ref = kmem_cache_alloc(btrfs_delayed_tree_ref_cachep, GFP_NOFS);
	if (!ref)
		return -ENOMEM;

	head_ref = kmem_cache_alloc(btrfs_delayed_ref_head_cachep, GFP_NOFS);
	if (!head_ref)
		goto free_ref;

	if (fs_info->quota_enabled && is_fstree(ref_root)) {
		record = kmalloc(sizeof(*record), GFP_NOFS);
		if (!record)
			goto free_head_ref;
	}

	head_ref->extent_op = extent_op;

	delayed_refs = &trans->transaction->delayed_refs;
	spin_lock(&delayed_refs->lock);

	/*
	 * insert both the head node and the new ref without dropping
	 * the spin lock
	 */
	head_ref = add_delayed_ref_head(fs_info, trans, &head_ref->node, record,
					bytenr, num_bytes, action, 0);

	add_delayed_tree_ref(fs_info, trans, head_ref, &ref->node, bytenr,
			     num_bytes, parent, ref_root, level, action);
	spin_unlock(&delayed_refs->lock);

	return 0;

free_head_ref:
	kmem_cache_free(btrfs_delayed_ref_head_cachep, head_ref);
free_ref:
	kmem_cache_free(btrfs_delayed_tree_ref_cachep, ref);

	return -ENOMEM;
}
