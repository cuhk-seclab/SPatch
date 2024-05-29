static int rbd_dev_probe_parent(struct rbd_device *rbd_dev, int depth)
{
	struct rbd_device *parent = NULL;
	int ret;

	if (!rbd_dev->parent_spec)
		return 0;

	if (++depth > RBD_MAX_PARENT_CHAIN_LEN) {
		pr_info("parent chain is too long (%d)\n", depth);
		ret = -EINVAL;
		goto out_err;
	}

	parent = rbd_dev_create(rbd_dev->rbd_client, rbd_dev->parent_spec,
				NULL);
	if (!parent) {
		ret = -ENOMEM;
		goto out_err;
	}

	/*
	 * Images related by parent/child relationships always share
	 * rbd_client and spec/parent_spec, so bump their refcounts.
	 */
	__rbd_get_client(rbd_dev->rbd_client);
	rbd_spec_get(rbd_dev->parent_spec);

	ret = rbd_dev_image_probe(parent, depth);
	if (ret < 0)
		goto out_err;

	rbd_dev->parent = parent;
	atomic_set(&rbd_dev->parent_ref, 1);
	return 0;

out_err:
	rbd_dev_unparent(rbd_dev);
	if (parent)
		rbd_dev_destroy(parent);
	return ret;
}
