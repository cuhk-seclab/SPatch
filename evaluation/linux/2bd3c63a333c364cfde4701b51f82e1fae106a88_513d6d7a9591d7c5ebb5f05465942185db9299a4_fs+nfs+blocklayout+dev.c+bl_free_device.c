static void
bl_free_device(struct pnfs_block_dev *dev)
{
	if (dev->nr_children) {
		int i;

		for (i = 0; i < dev->nr_children; i++)
			bl_free_device(&dev->children[i]);
		kfree(dev->children);
	} else {
		if (dev->bdev)
			blkdev_put(dev->bdev, FMODE_READ | FMODE_WRITE);
	}
}
