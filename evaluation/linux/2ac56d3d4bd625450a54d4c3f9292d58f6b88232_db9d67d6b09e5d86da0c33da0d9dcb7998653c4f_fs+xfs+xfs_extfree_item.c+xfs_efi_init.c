struct xfs_efi_log_item *
xfs_efi_init(
	struct xfs_mount	*mp,
	uint			nextents)

{
	struct xfs_efi_log_item	*efip;
	uint			size;

	ASSERT(nextents > 0);
	if (nextents > XFS_EFI_MAX_FAST_EXTENTS) {
		size = (uint)(sizeof(xfs_efi_log_item_t) +
			((nextents - 1) * sizeof(xfs_extent_t)));
		efip = kmem_zalloc(size, KM_SLEEP);
	} else {
		efip = kmem_zone_zalloc(xfs_efi_zone, KM_SLEEP);
	}

	xfs_log_item_init(mp, &efip->efi_item, XFS_LI_EFI, &xfs_efi_item_ops);
	efip->efi_format.efi_nextents = nextents;
	efip->efi_format.efi_id = (uintptr_t)(void *)efip;
	atomic_set(&efip->efi_next_extent, 0);
	atomic_set(&efip->efi_refcount, 2);

	return efip;
}
