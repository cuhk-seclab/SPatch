static ssize_t node_read_distance(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int nid = dev->id;
	int len = 0;
	int i;

	/*
	 * buf is currently PAGE_SIZE in length and each node needs 4 chars
	 * at the most (distance + space or newline).
	 */
	BUILD_BUG_ON(MAX_NUMNODES * 4 > PAGE_SIZE);

	for_each_online_node(i)
		len += sprintf(buf + len, "%s%d", i ? " " : "", node_distance(nid, i));

	len += sprintf(buf + len, "\n");
	return len;
}
