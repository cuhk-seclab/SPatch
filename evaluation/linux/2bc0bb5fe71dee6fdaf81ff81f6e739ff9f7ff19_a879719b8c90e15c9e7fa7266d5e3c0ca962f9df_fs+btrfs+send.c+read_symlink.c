static int read_symlink(struct btrfs_root *root,
			u64 ino,
			struct fs_path *dest)
{
	int ret;
	struct btrfs_path *path;
	struct btrfs_key key;
	struct btrfs_file_extent_item *ei;
	u8 type;
	u8 compression;
	unsigned long off;
	int len;

	path = alloc_path_for_send();
	if (!path)
		return -ENOMEM;

	key.objectid = ino;
	key.type = BTRFS_EXTENT_DATA_KEY;
	key.offset = 0;
	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		goto out;
	if (ret) {
		/*
		 * An empty symlink inode. Can happen in rare error paths when
		 * creating a symlink (transaction committed before the inode
		 * eviction handler removed the symlink inode items and a crash
		 * happened in between or the subvol was snapshoted in between).
		 * Print an informative message to dmesg/syslog so that the user
		 * can delete the symlink.
		 */
		btrfs_err(root->fs_info,
			  "Found empty symlink inode %llu at root %llu",
			  ino, root->root_key.objectid);
		ret = -EIO;
		goto out;
	}

	ei = btrfs_item_ptr(path->nodes[0], path->slots[0],
			struct btrfs_file_extent_item);
	type = btrfs_file_extent_type(path->nodes[0], ei);
	compression = btrfs_file_extent_compression(path->nodes[0], ei);
	BUG_ON(type != BTRFS_FILE_EXTENT_INLINE);
	BUG_ON(compression);

	off = btrfs_file_extent_inline_start(ei);
	len = btrfs_file_extent_inline_len(path->nodes[0], path->slots[0], ei);

	ret = fs_path_add_from_extent_buffer(dest, path->nodes[0], off, len);

out:
	btrfs_free_path(path);
	return ret;
}
