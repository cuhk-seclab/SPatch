int replace_file_extents(struct btrfs_trans_handle *trans,
			 struct reloc_control *rc,
			 struct btrfs_root *root,
			 struct extent_buffer *leaf)
{
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	struct inode *inode = NULL;
	u64 parent;
	u64 bytenr;
	u64 new_bytenr = 0;
	u64 num_bytes;
	u64 end;
	u32 nritems;
	u32 i;
	int ret = 0;
	int first = 1;
	int dirty = 0;

	if (rc->stage != UPDATE_DATA_PTRS)
		return 0;

	/* reloc trees always use full backref */
	if (root->root_key.objectid == BTRFS_TREE_RELOC_OBJECTID)
		parent = leaf->start;
	else
		parent = 0;

	nritems = btrfs_header_nritems(leaf);
	for (i = 0; i < nritems; i++) {
		cond_resched();
		btrfs_item_key_to_cpu(leaf, &key, i);
		if (key.type != BTRFS_EXTENT_DATA_KEY)
			continue;
		fi = btrfs_item_ptr(leaf, i, struct btrfs_file_extent_item);
		if (btrfs_file_extent_type(leaf, fi) ==
		    BTRFS_FILE_EXTENT_INLINE)
			continue;
		bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);
		num_bytes = btrfs_file_extent_disk_num_bytes(leaf, fi);
		if (bytenr == 0)
			continue;
		if (!in_block_group(bytenr, rc->block_group))
			continue;

		/*
		 * if we are modifying block in fs tree, wait for readpage
		 * to complete and drop the extent cache
		 */
		if (root->root_key.objectid != BTRFS_TREE_RELOC_OBJECTID) {
			if (first) {
				inode = find_next_inode(root, key.objectid);
				first = 0;
			} else if (inode && btrfs_ino(inode) < key.objectid) {
				btrfs_add_delayed_iput(inode);
				inode = find_next_inode(root, key.objectid);
			}
			if (inode && btrfs_ino(inode) == key.objectid) {
				end = key.offset +
				      btrfs_file_extent_num_bytes(leaf, fi);
				WARN_ON(!IS_ALIGNED(key.offset,
						    root->sectorsize));
				WARN_ON(!IS_ALIGNED(end, root->sectorsize));
				end--;
				ret = try_lock_extent(&BTRFS_I(inode)->io_tree,
						      key.offset, end);
				if (!ret)
					continue;

				btrfs_drop_extent_cache(inode, key.offset, end,
							1);
				unlock_extent(&BTRFS_I(inode)->io_tree,
					      key.offset, end);
			}
		}

		ret = get_new_location(rc->data_inode, &new_bytenr,
				       bytenr, num_bytes);
		if (ret) {
			/*
			 * Don't have to abort since we've not changed anything
			 * in the file extent yet.
			 */
			break;
		}

		btrfs_set_file_extent_disk_bytenr(leaf, fi, new_bytenr);
		dirty = 1;

		key.offset -= btrfs_file_extent_offset(leaf, fi);
		ret = btrfs_inc_extent_ref(trans, root, new_bytenr,
					   num_bytes, parent,
					   btrfs_header_owner(leaf),
					   key.objectid, key.offset, 1);
		if (ret) {
			btrfs_abort_transaction(trans, root, ret);
			break;
		}

		ret = btrfs_free_extent(trans, root, bytenr, num_bytes,
					parent, btrfs_header_owner(leaf),
					key.objectid, key.offset, 1);
		if (ret) {
			btrfs_abort_transaction(trans, root, ret);
			break;
		}
	}
	if (dirty)
		btrfs_mark_buffer_dirty(leaf);
	if (inode)
		btrfs_add_delayed_iput(inode);
	return ret;
}
