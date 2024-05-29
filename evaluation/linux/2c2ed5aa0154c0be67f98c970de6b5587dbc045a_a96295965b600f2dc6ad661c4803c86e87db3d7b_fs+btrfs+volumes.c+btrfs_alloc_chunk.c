int btrfs_alloc_chunk(struct btrfs_trans_handle *trans,
		      struct btrfs_root *extent_root, u64 type)
{
	u64 chunk_offset;

	ASSERT(mutex_is_locked(&extent_root->fs_info->chunk_mutex));
	chunk_offset = find_next_chunk(extent_root->fs_info);
	return __btrfs_alloc_chunk(trans, extent_root, chunk_offset, type);
}
