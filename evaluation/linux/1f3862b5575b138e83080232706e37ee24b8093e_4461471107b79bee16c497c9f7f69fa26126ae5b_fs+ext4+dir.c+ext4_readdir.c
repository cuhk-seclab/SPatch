static int ext4_readdir(struct file *file, struct dir_context *ctx)
{
	unsigned int offset;
	int i;
	struct ext4_dir_entry_2 *de;
	int err;
	struct inode *inode = file_inode(file);
	struct super_block *sb = inode->i_sb;
	int dir_has_error = 0;

	if (is_dx_dir(inode)) {
		err = ext4_dx_readdir(file, ctx);
		if (err != ERR_BAD_DX_DIR) {
			return err;
		}
		/*
		 * We don't set the inode dirty flag since it's not
		 * critical that it get flushed back to the disk.
		 */
		ext4_clear_inode_flag(file_inode(file),
				      EXT4_INODE_INDEX);
	}

	if (ext4_has_inline_data(inode)) {
		int has_inline_data = 1;
		int ret = ext4_read_inline_dir(file, ctx,
					   &has_inline_data);
		if (has_inline_data)
			return ret;
	}

	offset = ctx->pos & (sb->s_blocksize - 1);

	while (ctx->pos < inode->i_size) {
		struct ext4_map_blocks map;
		struct buffer_head *bh = NULL;

		map.m_lblk = ctx->pos >> EXT4_BLOCK_SIZE_BITS(sb);
		map.m_len = 1;
		err = ext4_map_blocks(NULL, inode, &map, 0);
		if (err > 0) {
			pgoff_t index = map.m_pblk >>
					(PAGE_CACHE_SHIFT - inode->i_blkbits);
			if (!ra_has_index(&file->f_ra, index))
				page_cache_sync_readahead(
					sb->s_bdev->bd_inode->i_mapping,
					&file->f_ra, file,
					index, 1);
			file->f_ra.prev_pos = (loff_t)index << PAGE_CACHE_SHIFT;
			bh = ext4_bread(NULL, inode, map.m_lblk, 0);
			if (IS_ERR(bh))
				return PTR_ERR(bh);
		}

		if (!bh) {
			if (!dir_has_error) {
				EXT4_ERROR_FILE(file, 0,
						"directory contains a "
						"hole at offset %llu",
					   (unsigned long long) ctx->pos);
				dir_has_error = 1;
			}
			/* corrupt size?  Maybe no more blocks to read */
			if (ctx->pos > inode->i_blocks << 9)
				break;
			ctx->pos += sb->s_blocksize - offset;
			continue;
		}

		/* Check the checksum */
		if (!buffer_verified(bh) &&
		    !ext4_dirent_csum_verify(inode,
				(struct ext4_dir_entry *)bh->b_data)) {
			EXT4_ERROR_FILE(file, 0, "directory fails checksum "
					"at offset %llu",
					(unsigned long long)ctx->pos);
			ctx->pos += sb->s_blocksize - offset;
			brelse(bh);
			continue;
		}
		set_buffer_verified(bh);

		/* If the dir block has changed since the last call to
		 * readdir(2), then we might be pointing to an invalid
		 * dirent right now.  Scan from the start of the block
		 * to make sure. */
		if (file->f_version != inode->i_version) {
			for (i = 0; i < sb->s_blocksize && i < offset; ) {
				de = (struct ext4_dir_entry_2 *)
					(bh->b_data + i);
				/* It's too expensive to do a full
				 * dirent test each time round this
				 * loop, but we do have to test at
				 * least that it is non-zero.  A
				 * failure will be detected in the
				 * dirent test below. */
				if (ext4_rec_len_from_disk(de->rec_len,
					sb->s_blocksize) < EXT4_DIR_REC_LEN(1))
					break;
				i += ext4_rec_len_from_disk(de->rec_len,
							    sb->s_blocksize);
			}
			offset = i;
			ctx->pos = (ctx->pos & ~(sb->s_blocksize - 1))
				| offset;
			file->f_version = inode->i_version;
		}

		while (ctx->pos < inode->i_size
		       && offset < sb->s_blocksize) {
			de = (struct ext4_dir_entry_2 *) (bh->b_data + offset);
			if (ext4_check_dir_entry(inode, file, de, bh,
						 bh->b_data, bh->b_size,
						 offset)) {
				/*
				 * On error, skip to the next block
				 */
				ctx->pos = (ctx->pos |
						(sb->s_blocksize - 1)) + 1;
				break;
			}
			offset += ext4_rec_len_from_disk(de->rec_len,
					sb->s_blocksize);
			if (le32_to_cpu(de->inode)) {
				if (!dir_emit(ctx, de->name,
						de->name_len,
						le32_to_cpu(de->inode),
						get_dtype(sb, de->file_type))) {
					brelse(bh);
					return 0;
				}
			}
			ctx->pos += ext4_rec_len_from_disk(de->rec_len,
						sb->s_blocksize);
		}
		offset = 0;
		brelse(bh);
		if (ctx->pos < inode->i_size) {
			if (!dir_relax(inode))
				return 0;
		}
	}
	return 0;
}
