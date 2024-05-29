static bool
pnfs_layoutgets_blocked(const struct pnfs_layout_hdr *lo)
{
	return lo->plh_block_lgets ||
		test_bit(NFS_LAYOUT_BULK_RECALL, &lo->plh_flags);
}
