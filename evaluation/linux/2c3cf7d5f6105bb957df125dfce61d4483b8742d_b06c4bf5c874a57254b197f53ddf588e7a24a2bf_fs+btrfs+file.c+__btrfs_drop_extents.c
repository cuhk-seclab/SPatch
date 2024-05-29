int __btrfs_drop_extents(struct btrfs_trans_handle *trans,
			 struct btrfs_root *root, struct inode *inode,
			 struct btrfs_path *path, u64 start, u64 end,
			 u64 *drop_end, int drop_cache,
			 int replace_extent,
			 u32 extent_item_size,
			 int *key_inserted)
{
	struct extent_buffer *leaf;
	struct btrfs_file_extent_item *fi;
	struct btrfs_key key;
	struct btrfs_key new_key;
	u64 ino = btrfs_ino(inode);
	u64 search_start = start;
	u64 disk_bytenr = 0;
	u64 num_bytes = 0;
	u64 extent_offset = 0;
	u64 extent_end = 0;
	int del_nr = 0;
	int del_slot = 0;
	int extent_type;
	int recow;
	int ret;
	int modify_tree = -1;
	int update_refs;
	int found = 0;
	int leafs_visited = 0;

	if (drop_cache)
		btrfs_drop_extent_cache(inode, start, end - 1, 0);

	if (start >= BTRFS_I(inode)->disk_i_size && !replace_extent)
		modify_tree = 0;

	update_refs = (test_bit(BTRFS_ROOT_REF_COWS, &root->state) ||
		       root == root->fs_info->tree_root);
	while (1) {
		recow = 0;
		ret = btrfs_lookup_file_extent(trans, root, path, ino,
					       search_start, modify_tree);
		if (ret < 0)
			break;
		if (ret > 0 && path->slots[0] > 0 && search_start == start) {
			leaf = path->nodes[0];
			btrfs_item_key_to_cpu(leaf, &key, path->slots[0] - 1);
			if (key.objectid == ino &&
			    key.type == BTRFS_EXTENT_DATA_KEY)
				path->slots[0]--;
		}
		ret = 0;
		leafs_visited++;
next_slot:
		leaf = path->nodes[0];
		if (path->slots[0] >= btrfs_header_nritems(leaf)) {
			BUG_ON(del_nr > 0);
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				break;
			if (ret > 0) {
				ret = 0;
				break;
			}
			leafs_visited++;
			leaf = path->nodes[0];
			recow = 1;
		}

		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
		if (key.objectid > ino ||
		    key.type > BTRFS_EXTENT_DATA_KEY || key.offset >= end)
			break;

		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);
		extent_type = btrfs_file_extent_type(leaf, fi);

		if (extent_type == BTRFS_FILE_EXTENT_REG ||
		    extent_type == BTRFS_FILE_EXTENT_PREALLOC) {
			disk_bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);
			num_bytes = btrfs_file_extent_disk_num_bytes(leaf, fi);
			extent_offset = btrfs_file_extent_offset(leaf, fi);
			extent_end = key.offset +
				btrfs_file_extent_num_bytes(leaf, fi);
		} else if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
			extent_end = key.offset +
				btrfs_file_extent_inline_len(leaf,
						     path->slots[0], fi);
		} else {
			WARN_ON(1);
			extent_end = search_start;
		}

		/*
		 * Don't skip extent items representing 0 byte lengths. They
		 * used to be created (bug) if while punching holes we hit
		 * -ENOSPC condition. So if we find one here, just ensure we
		 * delete it, otherwise we would insert a new file extent item
		 * with the same key (offset) as that 0 bytes length file
		 * extent item in the call to setup_items_for_insert() later
		 * in this function.
		 */
		if (extent_end == key.offset && extent_end >= search_start)
			goto delete_extent_item;

		if (extent_end <= search_start) {
			path->slots[0]++;
			goto next_slot;
		}

		found = 1;
		search_start = max(key.offset, start);
		if (recow || !modify_tree) {
			modify_tree = -1;
			btrfs_release_path(path);
			continue;
		}

		/*
		 *     | - range to drop - |
		 *  | -------- extent -------- |
		 */
		if (start > key.offset && end < extent_end) {
			BUG_ON(del_nr > 0);
			if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
				ret = -EOPNOTSUPP;
				break;
			}

			memcpy(&new_key, &key, sizeof(new_key));
			new_key.offset = start;
			ret = btrfs_duplicate_item(trans, root, path,
						   &new_key);
			if (ret == -EAGAIN) {
				btrfs_release_path(path);
				continue;
			}
			if (ret < 0)
				break;

			leaf = path->nodes[0];
			fi = btrfs_item_ptr(leaf, path->slots[0] - 1,
					    struct btrfs_file_extent_item);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							start - key.offset);

			fi = btrfs_item_ptr(leaf, path->slots[0],
					    struct btrfs_file_extent_item);

			extent_offset += start - key.offset;
			btrfs_set_file_extent_offset(leaf, fi, extent_offset);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							extent_end - start);
			btrfs_mark_buffer_dirty(leaf);

			if (update_refs && disk_bytenr > 0) {
				ret = btrfs_inc_extent_ref(trans, root,
						disk_bytenr, num_bytes, 0,
						root->root_key.objectid,
						new_key.objectid,
						start - extent_offset, 1);
				BUG_ON(ret); /* -ENOMEM */
			}
			key.offset = start;
		}
		/*
		 *  | ---- range to drop ----- |
		 *      | -------- extent -------- |
		 */
		if (start <= key.offset && end < extent_end) {
			if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
				ret = -EOPNOTSUPP;
				break;
			}

			memcpy(&new_key, &key, sizeof(new_key));
			new_key.offset = end;
			btrfs_set_item_key_safe(root->fs_info, path, &new_key);

			extent_offset += end - key.offset;
			btrfs_set_file_extent_offset(leaf, fi, extent_offset);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							extent_end - end);
			btrfs_mark_buffer_dirty(leaf);
			if (update_refs && disk_bytenr > 0)
				inode_sub_bytes(inode, end - key.offset);
			break;
		}

		search_start = extent_end;
		/*
		 *       | ---- range to drop ----- |
		 *  | -------- extent -------- |
		 */
		if (start > key.offset && end >= extent_end) {
			BUG_ON(del_nr > 0);
			if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
				ret = -EOPNOTSUPP;
				break;
			}

			btrfs_set_file_extent_num_bytes(leaf, fi,
							start - key.offset);
			btrfs_mark_buffer_dirty(leaf);
			if (update_refs && disk_bytenr > 0)
				inode_sub_bytes(inode, extent_end - start);
			if (end == extent_end)
				break;

			path->slots[0]++;
			goto next_slot;
		}

		/*
		 *  | ---- range to drop ----- |
		 *    | ------ extent ------ |
		 */
		if (start <= key.offset && end >= extent_end) {
delete_extent_item:
			if (del_nr == 0) {
				del_slot = path->slots[0];
				del_nr = 1;
			} else {
				BUG_ON(del_slot + del_nr != path->slots[0]);
				del_nr++;
			}

			if (update_refs &&
			    extent_type == BTRFS_FILE_EXTENT_INLINE) {
				inode_sub_bytes(inode,
						extent_end - key.offset);
				extent_end = ALIGN(extent_end,
						   root->sectorsize);
			} else if (update_refs && disk_bytenr > 0) {
				ret = btrfs_free_extent(trans, root,
						disk_bytenr, num_bytes, 0,
						root->root_key.objectid,
						key.objectid, key.offset -
						extent_offset, 0);
				BUG_ON(ret); /* -ENOMEM */
				inode_sub_bytes(inode,
						extent_end - key.offset);
			}

			if (end == extent_end)
				break;

			if (path->slots[0] + 1 < btrfs_header_nritems(leaf)) {
				path->slots[0]++;
				goto next_slot;
			}

			ret = btrfs_del_items(trans, root, path, del_slot,
					      del_nr);
			if (ret) {
				btrfs_abort_transaction(trans, root, ret);
				break;
			}

			del_nr = 0;
			del_slot = 0;

			btrfs_release_path(path);
			continue;
		}

		BUG_ON(1);
	}

	if (!ret && del_nr > 0) {
		/*
		 * Set path->slots[0] to first slot, so that after the delete
		 * if items are move off from our leaf to its immediate left or
		 * right neighbor leafs, we end up with a correct and adjusted
		 * path->slots[0] for our insertion (if replace_extent != 0).
		 */
		path->slots[0] = del_slot;
		ret = btrfs_del_items(trans, root, path, del_slot, del_nr);
		if (ret)
			btrfs_abort_transaction(trans, root, ret);
	}

	leaf = path->nodes[0];
	/*
	 * If btrfs_del_items() was called, it might have deleted a leaf, in
	 * which case it unlocked our path, so check path->locks[0] matches a
	 * write lock.
	 */
	if (!ret && replace_extent && leafs_visited == 1 &&
	    (path->locks[0] == BTRFS_WRITE_LOCK_BLOCKING ||
	     path->locks[0] == BTRFS_WRITE_LOCK) &&
	    btrfs_leaf_free_space(root, leaf) >=
	    sizeof(struct btrfs_item) + extent_item_size) {

		key.objectid = ino;
		key.type = BTRFS_EXTENT_DATA_KEY;
		key.offset = start;
		if (!del_nr && path->slots[0] < btrfs_header_nritems(leaf)) {
			struct btrfs_key slot_key;

			btrfs_item_key_to_cpu(leaf, &slot_key, path->slots[0]);
			if (btrfs_comp_cpu_keys(&key, &slot_key) > 0)
				path->slots[0]++;
		}
		setup_items_for_insert(root, path, &key,
				       &extent_item_size,
				       extent_item_size,
				       sizeof(struct btrfs_item) +
				       extent_item_size, 1);
		*key_inserted = 1;
	}

	if (!replace_extent || !(*key_inserted))
		btrfs_release_path(path);
	if (drop_end)
		*drop_end = found ? min(end, extent_end) : end;
	return ret;
}
