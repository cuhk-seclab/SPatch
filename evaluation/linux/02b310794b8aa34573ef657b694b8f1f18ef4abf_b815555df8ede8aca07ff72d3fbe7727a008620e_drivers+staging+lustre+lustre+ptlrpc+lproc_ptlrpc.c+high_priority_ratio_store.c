static ssize_t high_priority_ratio_store(struct kobject *kobj,
					 struct attribute *attr,
					 const char *buffer,
					 size_t count)
{
	struct ptlrpc_service *svc = container_of(kobj, struct ptlrpc_service,
						  srv_kobj);
	int rc;
	int val;

	rc = kstrtoint(buffer, 10, &val);
	if (rc < 0)
		return rc;

	if (val < 0)
		return -ERANGE;

	spin_lock(&svc->srv_lock);
	svc->srv_hpreq_ratio = val;
	spin_unlock(&svc->srv_lock);

	return count;
}
