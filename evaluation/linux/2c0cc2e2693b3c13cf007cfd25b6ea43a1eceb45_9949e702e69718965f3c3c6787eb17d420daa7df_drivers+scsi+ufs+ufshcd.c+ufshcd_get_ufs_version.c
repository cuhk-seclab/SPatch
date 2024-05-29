static inline u32 ufshcd_get_ufs_version(struct ufs_hba *hba)
{
	return ufshcd_readl(hba, REG_UFS_VERSION);
}
