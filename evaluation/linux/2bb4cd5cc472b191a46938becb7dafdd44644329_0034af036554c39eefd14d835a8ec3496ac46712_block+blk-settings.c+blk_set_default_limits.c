void blk_set_default_limits(struct queue_limits *lim)
{
	lim->max_segments = BLK_MAX_SEGMENTS;
	lim->max_integrity_segments = 0;
	lim->seg_boundary_mask = BLK_SEG_BOUNDARY_MASK;
	lim->max_segment_size = BLK_MAX_SEGMENT_SIZE;
	lim->max_sectors = lim->max_hw_sectors = BLK_SAFE_MAX_SECTORS;
	lim->chunk_sectors = 0;
	lim->max_write_same_sectors = 0;
	lim->max_discard_sectors = 0;
	lim->max_hw_discard_sectors = 0;
	lim->discard_granularity = 0;
	lim->discard_alignment = 0;
	lim->discard_misaligned = 0;
	lim->discard_zeroes_data = 0;
	lim->logical_block_size = lim->physical_block_size = lim->io_min = 512;
	lim->bounce_pfn = (unsigned long)(BLK_BOUNCE_ANY >> PAGE_SHIFT);
	lim->alignment_offset = 0;
	lim->io_opt = 0;
	lim->misaligned = 0;
	lim->cluster = 1;
}
