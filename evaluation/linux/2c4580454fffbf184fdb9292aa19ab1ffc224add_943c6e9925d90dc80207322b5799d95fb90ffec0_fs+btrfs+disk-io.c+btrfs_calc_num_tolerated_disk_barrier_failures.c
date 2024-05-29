int btrfs_calc_num_tolerated_disk_barrier_failures(
	struct btrfs_fs_info *fs_info)
{
	struct btrfs_ioctl_space_info space;
	struct btrfs_space_info *sinfo;
	u64 types[] = {BTRFS_BLOCK_GROUP_DATA,
		       BTRFS_BLOCK_GROUP_SYSTEM,
		       BTRFS_BLOCK_GROUP_METADATA,
		       BTRFS_BLOCK_GROUP_DATA | BTRFS_BLOCK_GROUP_METADATA};
	int i;
	int c;
	int num_tolerated_disk_barrier_failures =
		(int)fs_info->fs_devices->num_devices;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		struct btrfs_space_info *tmp;

		sinfo = NULL;
		rcu_read_lock();
		list_for_each_entry_rcu(tmp, &fs_info->space_info, list) {
			if (tmp->flags == types[i]) {
				sinfo = tmp;
				break;
			}
		}
		rcu_read_unlock();

		if (!sinfo)
			continue;

		down_read(&sinfo->groups_sem);
		for (c = 0; c < BTRFS_NR_RAID_TYPES; c++) {
			u64 flags;

			if (list_empty(&sinfo->block_groups[c]))
				continue;

			btrfs_get_block_group_info(&sinfo->block_groups[c],
						   &space);
			if (space.total_bytes == 0 || space.used_bytes == 0)
				continue;
			flags = space.flags;
			num_tolerated_disk_barrier_failures = min(
				num_tolerated_disk_barrier_failures,
				btrfs_get_num_tolerated_disk_barrier_failures(
			/*
				num_tolerated_disk_barrier_failures = 2;
		}
		up_read(&sinfo->groups_sem);
	}

	return num_tolerated_disk_barrier_failures;
}
