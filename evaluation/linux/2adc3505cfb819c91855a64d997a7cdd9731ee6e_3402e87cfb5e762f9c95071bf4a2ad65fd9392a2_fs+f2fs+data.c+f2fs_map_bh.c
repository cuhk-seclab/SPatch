static void f2fs_map_bh(struct super_block *sb, pgoff_t pgofs,
			struct extent_info *ei, struct buffer_head *bh_result)
{
	unsigned int blkbits = sb->s_blocksize_bits;
	size_t count;

	clear_buffer_new(bh_result);
	map_bh(bh_result, sb, ei->blk + pgofs - ei->fofs);
	count = ei->fofs + ei->len - pgofs;
	if (count < (UINT_MAX >> blkbits))
		bh_result->b_size = (count << blkbits);
	else
		bh_result->b_size = UINT_MAX;
}
