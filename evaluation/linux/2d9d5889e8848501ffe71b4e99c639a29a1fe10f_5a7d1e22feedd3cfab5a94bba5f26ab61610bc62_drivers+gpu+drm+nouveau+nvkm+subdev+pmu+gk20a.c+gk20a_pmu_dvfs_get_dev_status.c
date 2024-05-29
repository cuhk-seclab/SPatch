static int
gk20a_pmu_dvfs_get_dev_status(struct gk20a_pmu *pmu,
			      struct gk20a_pmu_dvfs_dev_status *status)
{
	status->busy = nv_rd32(pmu, 0x10a508 + (BUSY_SLOT * 0x10));
	status->total= nv_rd32(pmu, 0x10a508 + (CLK_SLOT * 0x10));
	return 0;
}
