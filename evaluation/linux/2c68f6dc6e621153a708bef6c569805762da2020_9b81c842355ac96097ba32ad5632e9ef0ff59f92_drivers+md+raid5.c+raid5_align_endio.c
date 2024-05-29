static void raid5_align_endio(struct bio *bi)
{
	struct bio* raid_bi  = bi->bi_private;
	struct mddev *mddev;
	struct r5conf *conf;
	struct md_rdev *rdev;
	int error = bi->bi_error;

	bio_put(bi);

	rdev = (void*)raid_bi->bi_next;
	raid_bi->bi_next = NULL;
	mddev = rdev->mddev;
	conf = mddev->private;

	rdev_dec_pending(rdev, conf->mddev);

	if (!error) {
		trace_block_bio_complete(bdev_get_queue(raid_bi->bi_bdev),
					 raid_bi, 0);
		bio_endio(raid_bi);
		if (atomic_dec_and_test(&conf->active_aligned_reads))
			wake_up(&conf->wait_for_quiescent);
		return;
	}

	pr_debug("raid5_align_endio : io error...handing IO for a retry\n");

	add_bio_to_retry(raid_bi, conf);
}
