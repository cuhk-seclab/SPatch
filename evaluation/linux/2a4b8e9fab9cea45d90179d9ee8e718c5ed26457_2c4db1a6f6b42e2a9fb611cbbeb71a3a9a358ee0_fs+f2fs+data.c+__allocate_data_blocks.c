static int __allocate_data_blocks(struct inode *inode, loff_t offset,
							size_t count)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct dnode_of_data dn;
	u64 start = F2FS_BYTES_TO_BLK(offset);
	u64 len = F2FS_BYTES_TO_BLK(count);
	bool allocated;
	u64 end_offset;
	int err = 0;

	while (len) {
		f2fs_lock_op(sbi);

		/* When reading holes, we need its node page */
		set_new_dnode(&dn, inode, NULL, NULL, 0);
		err = get_dnode_of_data(&dn, start, ALLOC_NODE);
		if (err)
			goto out;

		allocated = false;
		end_offset = ADDRS_PER_PAGE(dn.node_page, F2FS_I(inode));

		while (dn.ofs_in_node < end_offset && len) {
			block_t blkaddr;

			if (unlikely(f2fs_cp_error(sbi))) {
				err = -EIO;
				goto sync_out;
			}

			blkaddr = datablock_addr(dn.node_page, dn.ofs_in_node);
			if (blkaddr == NULL_ADDR || blkaddr == NEW_ADDR) {
				err = __allocate_data_block(&dn);
				if (err)
					goto sync_out;
				allocated = true;
			}
			len--;
			start++;
			dn.ofs_in_node++;
		}

		if (allocated)
			sync_inode_page(&dn);

		f2fs_put_dnode(&dn);
		f2fs_unlock_op(sbi);

		if (dn.node_changed)
			f2fs_balance_fs(sbi);
	}
	return err;

sync_out:
	if (allocated)
		sync_inode_page(&dn);
	f2fs_put_dnode(&dn);
out:
	f2fs_unlock_op(sbi);
	f2fs_balance_fs(sbi, dn.node_changed);
	return err;
}
