int btrfs_balance(struct btrfs_balance_control *bctl,
		  struct btrfs_ioctl_balance_args *bargs)
{
	struct btrfs_fs_info *fs_info = bctl->fs_info;
	u64 allowed;
	int mixed = 0;
	int ret;
	u64 num_devices;
	unsigned seq;

	if (btrfs_fs_closing(fs_info) ||
	    atomic_read(&fs_info->balance_pause_req) ||
	    atomic_read(&fs_info->balance_cancel_req)) {
		ret = -EINVAL;
		goto out;
	}

	allowed = btrfs_super_incompat_flags(fs_info->super_copy);
	if (allowed & BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS)
		mixed = 1;

	/*
	 * In case of mixed groups both data and meta should be picked,
	 * and identical options should be given for both of them.
	 */
	allowed = BTRFS_BALANCE_DATA | BTRFS_BALANCE_METADATA;
	if (mixed && (bctl->flags & allowed)) {
		if (!(bctl->flags & BTRFS_BALANCE_DATA) ||
		    !(bctl->flags & BTRFS_BALANCE_METADATA) ||
		    memcmp(&bctl->data, &bctl->meta, sizeof(bctl->data))) {
			btrfs_err(fs_info, "with mixed groups data and "
				   "metadata balance options must be the same");
			ret = -EINVAL;
			goto out;
		}
	}

	num_devices = fs_info->fs_devices->num_devices;
	btrfs_dev_replace_lock(&fs_info->dev_replace);
	if (btrfs_dev_replace_is_ongoing(&fs_info->dev_replace)) {
		BUG_ON(num_devices < 1);
		num_devices--;
	}
	btrfs_dev_replace_unlock(&fs_info->dev_replace);
	allowed = BTRFS_AVAIL_ALLOC_BIT_SINGLE;
	if (num_devices == 1)
		allowed |= BTRFS_BLOCK_GROUP_DUP;
	else if (num_devices > 1)
		allowed |= (BTRFS_BLOCK_GROUP_RAID0 | BTRFS_BLOCK_GROUP_RAID1);
	if (num_devices > 2)
		allowed |= BTRFS_BLOCK_GROUP_RAID5;
	if (num_devices > 3)
		allowed |= (BTRFS_BLOCK_GROUP_RAID10 |
			    BTRFS_BLOCK_GROUP_RAID6);
	if ((bctl->data.flags & BTRFS_BALANCE_ARGS_CONVERT) &&
	    (!alloc_profile_is_valid(bctl->data.target, 1) ||
	     (bctl->data.target & ~allowed))) {
		btrfs_err(fs_info, "unable to start balance with target "
			   "data profile %llu",
		       bctl->data.target);
		ret = -EINVAL;
		goto out;
	}
	if ((bctl->meta.flags & BTRFS_BALANCE_ARGS_CONVERT) &&
	    (!alloc_profile_is_valid(bctl->meta.target, 1) ||
	     (bctl->meta.target & ~allowed))) {
		btrfs_err(fs_info,
			   "unable to start balance with target metadata profile %llu",
		       bctl->meta.target);
		ret = -EINVAL;
		goto out;
	}
	if ((bctl->sys.flags & BTRFS_BALANCE_ARGS_CONVERT) &&
	    (!alloc_profile_is_valid(bctl->sys.target, 1) ||
	     (bctl->sys.target & ~allowed))) {
		btrfs_err(fs_info,
			   "unable to start balance with target system profile %llu",
		       bctl->sys.target);
		ret = -EINVAL;
		goto out;
	}

	/* allow dup'ed data chunks only in mixed mode */
	if (!mixed && (bctl->data.flags & BTRFS_BALANCE_ARGS_CONVERT) &&
	    (bctl->data.target & BTRFS_BLOCK_GROUP_DUP)) {
		btrfs_err(fs_info, "dup for data is not allowed");
		ret = -EINVAL;
		goto out;
	}

	/* allow to reduce meta or sys integrity only if force set */
	allowed = BTRFS_BLOCK_GROUP_DUP | BTRFS_BLOCK_GROUP_RAID1 |
			BTRFS_BLOCK_GROUP_RAID10 |
			BTRFS_BLOCK_GROUP_RAID5 |
			BTRFS_BLOCK_GROUP_RAID6;
	do {
		seq = read_seqbegin(&fs_info->profiles_lock);

		if (((bctl->sys.flags & BTRFS_BALANCE_ARGS_CONVERT) &&
		     (fs_info->avail_system_alloc_bits & allowed) &&
		     !(bctl->sys.target & allowed)) ||
		    ((bctl->meta.flags & BTRFS_BALANCE_ARGS_CONVERT) &&
		     (fs_info->avail_metadata_alloc_bits & allowed) &&
		     !(bctl->meta.target & allowed))) {
			if (bctl->flags & BTRFS_BALANCE_FORCE) {
				btrfs_info(fs_info, "force reducing metadata integrity");
			} else {
				btrfs_err(fs_info, "balance will reduce metadata "
					   "integrity, use force if you want this");
				ret = -EINVAL;
				goto out;
			}
		}
	} while (read_seqretry(&fs_info->profiles_lock, seq));

	if (bctl->sys.flags & BTRFS_BALANCE_ARGS_CONVERT) {
		fs_info->num_tolerated_disk_barrier_failures = min(
			btrfs_calc_num_tolerated_disk_barrier_failures(fs_info),
			btrfs_get_num_tolerated_disk_barrier_failures(
				bctl->sys.target));
	}

	ret = insert_balance_item(fs_info->tree_root, bctl);
	if (ret && ret != -EEXIST)
		goto out;

	if (!(bctl->flags & BTRFS_BALANCE_RESUME)) {
		BUG_ON(ret == -EEXIST);
		set_balance_control(bctl);
	} else {
		BUG_ON(ret != -EEXIST);
		spin_lock(&fs_info->balance_lock);
		update_balance_args(bctl);
		spin_unlock(&fs_info->balance_lock);
	}

	atomic_inc(&fs_info->balance_running);
	mutex_unlock(&fs_info->balance_mutex);

	ret = __btrfs_balance(fs_info);

	mutex_lock(&fs_info->balance_mutex);
	atomic_dec(&fs_info->balance_running);

	if (bctl->sys.flags & BTRFS_BALANCE_ARGS_CONVERT) {
		fs_info->num_tolerated_disk_barrier_failures =
			btrfs_calc_num_tolerated_disk_barrier_failures(fs_info);
	}

	if (bargs) {
		memset(bargs, 0, sizeof(*bargs));
		update_ioctl_balance_args(fs_info, 0, bargs);
	}

	if ((ret && ret != -ECANCELED && ret != -ENOSPC) ||
	    balance_need_close(fs_info)) {
		__cancel_balance(fs_info);
	}

	wake_up(&fs_info->balance_wait_q);

	return ret;
out:
	if (bctl->flags & BTRFS_BALANCE_RESUME)
		__cancel_balance(fs_info);
	else {
		kfree(bctl);
		atomic_set(&fs_info->mutually_exclusive_operation_running, 0);
	}
	return ret;
}
