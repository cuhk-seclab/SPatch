static int read_one_dev(struct btrfs_root *root,
			struct extent_buffer *leaf,
			struct btrfs_dev_item *dev_item)
{
	struct btrfs_fs_devices *fs_devices = root->fs_info->fs_devices;
	struct btrfs_device *device;
	u64 devid;
	int ret;
	u8 fs_uuid[BTRFS_UUID_SIZE];
	u8 dev_uuid[BTRFS_UUID_SIZE];

	devid = btrfs_device_id(leaf, dev_item);
	read_extent_buffer(leaf, dev_uuid, btrfs_device_uuid(dev_item),
			   BTRFS_UUID_SIZE);
	read_extent_buffer(leaf, fs_uuid, btrfs_device_fsid(dev_item),
			   BTRFS_UUID_SIZE);

	if (memcmp(fs_uuid, root->fs_info->fsid, BTRFS_UUID_SIZE)) {
		fs_devices = open_seed_devices(root, fs_uuid);
		if (IS_ERR(fs_devices))
			return PTR_ERR(fs_devices);
	}

	device = btrfs_find_device(root->fs_info, devid, dev_uuid, fs_uuid);
	if (!device) {
		if (!btrfs_test_opt(root, DEGRADED))
			return -EIO;

		device = add_missing_dev(root, fs_devices, devid, dev_uuid);
		if (!device)
			return -ENOMEM;
		btrfs_warn(root->fs_info, "devid %llu uuid %pU missing",
				devid, dev_uuid);
	} else {
		if (!device->bdev && !btrfs_test_opt(root, DEGRADED))
			return -EIO;

		if(!device->bdev && !device->missing) {
			/*
			 * this happens when a device that was properly setup
			 * in the device info lists suddenly goes bad.
			 * device->bdev is NULL, and so we have to set
			 * device->missing to one here
			 */
			device->fs_devices->missing_devices++;
			device->missing = 1;
		}

		/* Move the device to its own fs_devices */
		if (device->fs_devices != fs_devices) {
			ASSERT(device->missing);

			list_move(&device->dev_list, &fs_devices->devices);
			device->fs_devices->num_devices--;
			fs_devices->num_devices++;

			device->fs_devices->missing_devices--;
			fs_devices->missing_devices++;

			device->fs_devices = fs_devices;
		}
	}

	if (device->fs_devices != root->fs_info->fs_devices) {
		BUG_ON(device->writeable);
		if (device->generation !=
		    btrfs_device_generation(leaf, dev_item))
			return -EINVAL;
	}

	fill_device_from_item(leaf, dev_item, device);
	device->in_fs_metadata = 1;
	if (device->writeable && !device->is_tgtdev_for_dev_replace) {
		device->fs_devices->total_rw_bytes += device->total_bytes;
		spin_lock(&root->fs_info->free_chunk_lock);
		root->fs_info->free_chunk_space += device->total_bytes -
			device->bytes_used;
		spin_unlock(&root->fs_info->free_chunk_lock);
	}
	ret = 0;
	return ret;
}
