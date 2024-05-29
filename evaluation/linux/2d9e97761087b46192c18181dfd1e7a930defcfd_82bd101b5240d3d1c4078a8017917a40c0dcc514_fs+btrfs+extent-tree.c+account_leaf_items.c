static int account_leaf_items(struct btrfs_trans_handle *trans,
			      struct btrfs_root *root,
			      struct extent_buffer *eb)
{
	int nr = btrfs_header_nritems(eb);
	int i, extent_type;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	u64 bytenr, num_bytes;

	if (!root->fs_info->quota_enabled)
		return 0;
	for (i = 0; i < nr; i++) {
		btrfs_item_key_to_cpu(eb, &key, i);

		if (key.type != BTRFS_EXTENT_DATA_KEY)
			continue;

		fi = btrfs_item_ptr(eb, i, struct btrfs_file_extent_item);
		/* filter out non qgroup-accountable extents  */
		extent_type = btrfs_file_extent_type(eb, fi);

		if (extent_type == BTRFS_FILE_EXTENT_INLINE)
			continue;

		bytenr = btrfs_file_extent_disk_bytenr(eb, fi);
		if (!bytenr)
			continue;

		num_bytes = btrfs_file_extent_disk_num_bytes(eb, fi);
		ret = record_one_subtree_extent(trans, root, bytenr, num_bytes);
		if (ret)
	}
	return 0;
}
