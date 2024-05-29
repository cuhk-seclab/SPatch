static void
xfs_map_direct(
	struct inode		*inode,
	struct buffer_head	*bh_result,
	struct xfs_bmbt_irec	*imap,
	xfs_off_t		offset)
{
	struct xfs_ioend	*ioend;
	xfs_off_t		size = bh_result->b_size;
	int			type;

	if (ISUNWRITTEN(imap))
		type = XFS_IO_UNWRITTEN;
	else
		type = XFS_IO_OVERWRITE;

	trace_xfs_gbmap_direct(XFS_I(inode), offset, size, type, imap);

	if (bh_result->b_private) {
		ioend = bh_result->b_private;
		ASSERT(ioend->io_size > 0);
		ASSERT(offset >= ioend->io_offset);
		if (offset + size > ioend->io_offset + ioend->io_size)
			ioend->io_size = offset - ioend->io_offset + size;

		if (type == XFS_IO_UNWRITTEN && type != ioend->io_type)
			ioend->io_type = XFS_IO_UNWRITTEN;

		trace_xfs_gbmap_direct_update(XFS_I(inode), ioend->io_offset,
					      ioend->io_size, ioend->io_type,
					      imap);
	} else {
		ioend = xfs_alloc_ioend(inode, type);
		ioend->io_offset = offset;
		ioend->io_size = size;
		bh_result->b_private = ioend;

		trace_xfs_gbmap_direct_new(XFS_I(inode), offset, size, type,
					   imap);
	}

	if (ioend->io_type == XFS_IO_UNWRITTEN)
		set_buffer_defer_completion(bh_result);
}
