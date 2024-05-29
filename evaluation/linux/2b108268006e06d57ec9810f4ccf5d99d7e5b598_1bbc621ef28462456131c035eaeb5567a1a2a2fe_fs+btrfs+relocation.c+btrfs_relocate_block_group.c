int btrfs_relocate_block_group(struct btrfs_root *extent_root, u64 group_start)
{
	struct btrfs_fs_info *fs_info = extent_root->fs_info;
	struct reloc_control *rc;
	struct inode *inode;
	struct btrfs_path *path;
	int ret;
	int rw = 0;
	int err = 0;

	rc = alloc_reloc_control(fs_info);
	if (!rc)
		return -ENOMEM;

	rc->extent_root = extent_root;

	rc->block_group = btrfs_lookup_block_group(fs_info, group_start);
	BUG_ON(!rc->block_group);

	if (!rc->block_group->ro) {
		ret = btrfs_set_block_group_ro(extent_root, rc->block_group);
		if (ret) {
			err = ret;
			goto out;
		}
		rw = 1;
	}

	path = btrfs_alloc_path();
	if (!path) {
		err = -ENOMEM;
		goto out;
	}

	inode = lookup_free_space_inode(fs_info->tree_root, rc->block_group,
					path);
	btrfs_free_path(path);

	if (!IS_ERR(inode))
		ret = delete_block_group_cache(fs_info, inode, 0);
	else
		ret = PTR_ERR(inode);

	if (ret && ret != -ENOENT) {
		err = ret;
		goto out;
	}

	rc->data_inode = create_reloc_inode(fs_info, rc->block_group);
	if (IS_ERR(rc->data_inode)) {
		err = PTR_ERR(rc->data_inode);
		rc->data_inode = NULL;
		goto out;
	}

	btrfs_info(extent_root->fs_info, "relocating block group %llu flags %llu",
	       rc->block_group->key.objectid, rc->block_group->flags);

	ret = btrfs_start_delalloc_roots(fs_info, 0, -1);
	if (ret < 0) {
		err = ret;
		goto out;
	}
	btrfs_wait_ordered_roots(fs_info, -1);

	while (1) {
		mutex_lock(&fs_info->cleaner_mutex);
		ret = relocate_block_group(rc);
		mutex_unlock(&fs_info->cleaner_mutex);
		if (ret < 0) {
			err = ret;
			goto out;
		}

		if (rc->extents_found == 0)
			break;

		btrfs_info(extent_root->fs_info, "found %llu extents",
			rc->extents_found);

		if (rc->stage == MOVE_DATA_EXTENTS && rc->found_file_extent) {
			ret = btrfs_wait_ordered_range(rc->data_inode, 0,
						       (u64)-1);
			if (ret) {
				err = ret;
				goto out;
			}
			invalidate_mapping_pages(rc->data_inode->i_mapping,
						 0, -1);
			rc->stage = UPDATE_DATA_PTRS;
		}
	}

	WARN_ON(rc->block_group->pinned > 0);
	WARN_ON(rc->block_group->reserved > 0);
	WARN_ON(btrfs_block_group_used(&rc->block_group->item) > 0);
out:
	if (err && rw)
		btrfs_set_block_group_rw(extent_root, rc->block_group);
	iput(rc->data_inode);
	btrfs_put_block_group(rc->block_group);
	kfree(rc);
	return err;
}
