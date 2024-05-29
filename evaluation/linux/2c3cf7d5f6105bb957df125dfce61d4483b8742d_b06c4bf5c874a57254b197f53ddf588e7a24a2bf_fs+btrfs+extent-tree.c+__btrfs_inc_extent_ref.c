static int __btrfs_inc_extent_ref(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root,
				  struct btrfs_delayed_ref_node *node,
				  u64 parent, u64 root_objectid,
				  u64 owner, u64 offset, int refs_to_add,
				  struct btrfs_delayed_extent_op *extent_op)
{
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct btrfs_path *path;
	struct extent_buffer *leaf;
	struct btrfs_extent_item *item;
	struct btrfs_key key;
	u64 bytenr = node->bytenr;
	u64 num_bytes = node->num_bytes;
	u64 refs;
	int ret;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->reada = 1;
	path->leave_spinning = 1;
	/* this will setup the path even if it fails to insert the back ref */
	ret = insert_inline_extent_backref(trans, fs_info->extent_root, path,
					   bytenr, num_bytes, parent,
					   root_objectid, owner, offset,
					   refs_to_add, extent_op);
	if ((ret < 0 && ret != -EAGAIN) || !ret)
		goto out;

	/*
	 * Ok we had -EAGAIN which means we didn't have space to insert and
	 * inline extent ref, so just update the reference count and add a
	 * normal backref.
	 */
	leaf = path->nodes[0];
	btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
	item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_extent_item);
	refs = btrfs_extent_refs(leaf, item);
	btrfs_set_extent_refs(leaf, item, refs + refs_to_add);
	if (extent_op)
		__run_delayed_extent_op(extent_op, leaf, item);

	btrfs_mark_buffer_dirty(leaf);
	btrfs_release_path(path);

	path->reada = 1;
	path->leave_spinning = 1;
	/* now insert the actual backref */
	ret = insert_extent_backref(trans, root->fs_info->extent_root,
				    path, bytenr, parent, root_objectid,
				    owner, offset, refs_to_add);
	if (ret)
		btrfs_abort_transaction(trans, root, ret);
out:
	btrfs_free_path(path);
	return ret;
}
