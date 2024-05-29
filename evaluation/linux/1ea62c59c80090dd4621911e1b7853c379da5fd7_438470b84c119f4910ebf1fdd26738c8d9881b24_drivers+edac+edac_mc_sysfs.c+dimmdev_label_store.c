static ssize_t dimmdev_label_store(struct device *dev,
				   struct device_attribute *mattr,
				   const char *data,
				   size_t count)
{
	struct dimm_info *dimm = to_dimm(dev);
	size_t copy_count = count;

	if (count == 0)
		return -EINVAL;

	if (data[count - 1] == '\0' || data[count - 1] == '\n')
		copy_count -= 1;

	if (copy_count >= sizeof(dimm->label))
		return -EINVAL;

	strncpy(dimm->label, data, copy_count);
	dimm->label[copy_count] = '\0';

	return count;
}
