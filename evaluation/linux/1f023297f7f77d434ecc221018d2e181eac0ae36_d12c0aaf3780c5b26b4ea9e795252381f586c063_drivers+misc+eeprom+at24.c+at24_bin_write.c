static ssize_t at24_bin_write(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count)
{
	struct at24_data *at24;

	at24 = dev_get_drvdata(container_of(kobj, struct device, kobj));
	return at24_write(at24, buf, off, count);
}
