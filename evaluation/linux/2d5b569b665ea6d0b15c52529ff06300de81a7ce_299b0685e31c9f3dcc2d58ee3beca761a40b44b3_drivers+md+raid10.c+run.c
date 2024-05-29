static int run(struct mddev *mddev)
{
	struct r10conf *conf;
	int i, disk_idx, chunk_size;
	struct raid10_info *disk;
	struct md_rdev *rdev;
	sector_t size;
	sector_t min_offset_diff = 0;
	int first = 1;
	bool discard_supported = false;

	if (mddev->private == NULL) {
		conf = setup_conf(mddev);
		if (IS_ERR(conf))
			return PTR_ERR(conf);
		mddev->private = conf;
	}
	conf = mddev->private;
	if (!conf)
		goto out;

	mddev->thread = conf->thread;
	conf->thread = NULL;

	chunk_size = mddev->chunk_sectors << 9;
	if (mddev->queue) {
		blk_queue_max_discard_sectors(mddev->queue,
					      mddev->chunk_sectors);
		blk_queue_max_write_same_sectors(mddev->queue, 0);
		blk_queue_io_min(mddev->queue, chunk_size);
		if (conf->geo.raid_disks % conf->geo.near_copies)
			blk_queue_io_opt(mddev->queue, chunk_size * conf->geo.raid_disks);
		else
			blk_queue_io_opt(mddev->queue, chunk_size *
					 (conf->geo.raid_disks / conf->geo.near_copies));
	}

	rdev_for_each(rdev, mddev) {
		long long diff;
		struct request_queue *q;

		disk_idx = rdev->raid_disk;
		if (disk_idx < 0)
			continue;
		if (disk_idx >= conf->geo.raid_disks &&
		    disk_idx >= conf->prev.raid_disks)
			continue;
		disk = conf->mirrors + disk_idx;

		if (test_bit(Replacement, &rdev->flags)) {
			if (disk->replacement)
				goto out_free_conf;
			disk->replacement = rdev;
		} else {
			if (disk->rdev)
				goto out_free_conf;
			disk->rdev = rdev;
		}
		q = bdev_get_queue(rdev->bdev);
		if (q->merge_bvec_fn)
			mddev->merge_check_needed = 1;
		diff = (rdev->new_data_offset - rdev->data_offset);
		if (!mddev->reshape_backwards)
			diff = -diff;
		if (diff < 0)
			diff = 0;
		if (first || diff < min_offset_diff)
			min_offset_diff = diff;

		if (mddev->gendisk)
			disk_stack_limits(mddev->gendisk, rdev->bdev,
					  rdev->data_offset << 9);

		disk->head_position = 0;

		if (blk_queue_discard(bdev_get_queue(rdev->bdev)))
			discard_supported = true;
	}

	if (mddev->queue) {
		if (discard_supported)
			queue_flag_set_unlocked(QUEUE_FLAG_DISCARD,
						mddev->queue);
		else
			queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD,
						  mddev->queue);
	}
	/* need to check that every block has at least one working mirror */
	if (!enough(conf, -1)) {
		printk(KERN_ERR "md/raid10:%s: not enough operational mirrors.\n",
		       mdname(mddev));
		goto out_free_conf;
	}

	if (conf->reshape_progress != MaxSector) {
		/* must ensure that shape change is supported */
		if (conf->geo.far_copies != 1 &&
		    conf->geo.far_offset == 0)
			goto out_free_conf;
		if (conf->prev.far_copies != 1 &&
		    conf->prev.far_offset == 0)
			goto out_free_conf;
	}

	mddev->degraded = 0;
	for (i = 0;
	     i < conf->geo.raid_disks
		     || i < conf->prev.raid_disks;
	     i++) {

		disk = conf->mirrors + i;

		if (!disk->rdev && disk->replacement) {
			/* The replacement is all we have - use it */
			disk->rdev = disk->replacement;
			disk->replacement = NULL;
			clear_bit(Replacement, &disk->rdev->flags);
		}

		if (!disk->rdev ||
		    !test_bit(In_sync, &disk->rdev->flags)) {
			disk->head_position = 0;
			mddev->degraded++;
			if (disk->rdev &&
			    disk->rdev->saved_raid_disk < 0)
				conf->fullsync = 1;
		}
		disk->recovery_disabled = mddev->recovery_disabled - 1;
	}

	if (mddev->recovery_cp != MaxSector)
		printk(KERN_NOTICE "md/raid10:%s: not clean"
		       " -- starting background reconstruction\n",
		       mdname(mddev));
	printk(KERN_INFO
		"md/raid10:%s: active with %d out of %d devices\n",
		mdname(mddev), conf->geo.raid_disks - mddev->degraded,
		conf->geo.raid_disks);
	/*
	 * Ok, everything is just fine now
	 */
	mddev->dev_sectors = conf->dev_sectors;
	size = raid10_size(mddev, 0, 0);
	md_set_array_sectors(mddev, size);
	mddev->resync_max_sectors = size;

	if (mddev->queue) {
		int stripe = conf->geo.raid_disks *
			((mddev->chunk_sectors << 9) / PAGE_SIZE);

		/* Calculate max read-ahead size.
		 * We need to readahead at least twice a whole stripe....
		 * maybe...
		 */
		stripe /= conf->geo.near_copies;
		if (mddev->queue->backing_dev_info.ra_pages < 2 * stripe)
			mddev->queue->backing_dev_info.ra_pages = 2 * stripe;
	}

	if (md_integrity_register(mddev))
		goto out_free_conf;

	if (conf->reshape_progress != MaxSector) {
		unsigned long before_length, after_length;

		before_length = ((1 << conf->prev.chunk_shift) *
				 conf->prev.far_copies);
		after_length = ((1 << conf->geo.chunk_shift) *
				conf->geo.far_copies);

		if (max(before_length, after_length) > min_offset_diff) {
			/* This cannot work */
			printk("md/raid10: offset difference not enough to continue reshape\n");
			goto out_free_conf;
		}
		conf->offset_diff = min_offset_diff;

		clear_bit(MD_RECOVERY_SYNC, &mddev->recovery);
		clear_bit(MD_RECOVERY_CHECK, &mddev->recovery);
		set_bit(MD_RECOVERY_RESHAPE, &mddev->recovery);
		set_bit(MD_RECOVERY_RUNNING, &mddev->recovery);
		mddev->sync_thread = md_register_thread(md_do_sync, mddev,
							"reshape");
	}

	return 0;

out_free_conf:
	md_unregister_thread(&mddev->thread);
	if (conf->r10bio_pool)
		mempool_destroy(conf->r10bio_pool);
	safe_put_page(conf->tmppage);
	kfree(conf->mirrors);
	kfree(conf);
	mddev->private = NULL;
out:
	return -EIO;
}
