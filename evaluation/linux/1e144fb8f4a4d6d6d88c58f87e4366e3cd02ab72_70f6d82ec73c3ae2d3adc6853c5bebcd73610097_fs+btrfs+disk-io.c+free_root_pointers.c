static void free_root_pointers(struct btrfs_fs_info *info, int chunk_root)
{
	free_root_extent_buffers(info->tree_root);

	free_root_extent_buffers(info->dev_root);
	free_root_extent_buffers(info->extent_root);
	free_root_extent_buffers(info->csum_root);
	free_root_extent_buffers(info->quota_root);
	free_root_extent_buffers(info->uuid_root);
	if (chunk_root)
		free_root_extent_buffers(info->chunk_root);
	free_root_extent_buffers(info->free_space_root);
}
