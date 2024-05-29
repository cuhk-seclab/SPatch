static int amd_iommu_add_device(struct device *dev)
{
	struct iommu_dev_data *dev_data;
	struct iommu_domain *domain;
	struct amd_iommu *iommu;
	u16 devid;
	int ret;

	if (!check_device(dev) || get_dev_data(dev))
		return 0;

	devid = get_device_id(dev);
	iommu = amd_iommu_rlookup_table[devid];

	ret = iommu_init_device(dev);
	if (ret) {
		if (ret != -ENOTSUPP)
			pr_err("Failed to initialize device %s - trying to proceed anyway\n",
				dev_name(dev));

		iommu_ignore_device(dev);
		dev->archdata.dma_ops = &nommu_dma_ops;
		goto out;
	}
	init_iommu_group(dev);

	dev_data = get_dev_data(dev);

	BUG_ON(!dev_data);

	if (iommu_pass_through || dev_data->iommu_v2)
		iommu_request_dm_for_dev(dev);

	/* Domains are initialized for this device - have a look what we ended up with */
	domain = iommu_get_domain_for_dev(dev);
	if (domain->type == IOMMU_DOMAIN_IDENTITY)
		dev_data->passthrough = true;
	else
		dev->archdata.dma_ops = &amd_iommu_dma_ops;

out:
	iommu_completion_wait(iommu);

	return 0;
}
