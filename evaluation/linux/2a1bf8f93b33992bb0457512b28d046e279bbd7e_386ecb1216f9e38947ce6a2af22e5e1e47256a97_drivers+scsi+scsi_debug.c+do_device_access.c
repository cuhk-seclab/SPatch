static int
do_device_access(struct scsi_cmnd *scmd, u64 lba, u32 num, bool do_write)
{
	int ret;
	u64 block, rest = 0;
	struct scsi_data_buffer *sdb;
	enum dma_data_direction dir;

	if (do_write) {
		sdb = scsi_out(scmd);
		dir = DMA_TO_DEVICE;
	} else {
		sdb = scsi_in(scmd);
		dir = DMA_FROM_DEVICE;
	}

	if (!sdb->length)
		return 0;
	if (!(scsi_bidi_cmnd(scmd) || scmd->sc_data_direction == dir))
		return -1;

	block = do_div(lba, sdebug_store_sectors);
	if (block + num > sdebug_store_sectors)
		rest = block + num - sdebug_store_sectors;

	ret = sg_copy_buffer(sdb->table.sgl, sdb->table.nents,
		   fake_storep + (block * scsi_debug_sector_size),
		   (num - rest) * scsi_debug_sector_size, 0, do_write);
	if (ret != (num - rest) * scsi_debug_sector_size)
		return ret;

	if (rest) {
		ret += sg_copy_buffer(sdb->table.sgl, sdb->table.nents,
			    fake_storep, rest * scsi_debug_sector_size,
			    (num - rest) * scsi_debug_sector_size, do_write);
	}

	return ret;
}
