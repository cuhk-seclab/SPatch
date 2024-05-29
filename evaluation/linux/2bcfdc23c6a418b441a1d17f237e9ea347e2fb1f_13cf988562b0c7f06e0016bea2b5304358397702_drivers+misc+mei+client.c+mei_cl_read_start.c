int mei_cl_read_start(struct mei_cl *cl, size_t length, struct file *fp)
{
	struct mei_device *dev;
	struct mei_cl_cb *cb;
	int rets;

	if (WARN_ON(!cl || !cl->dev))
		return -ENODEV;

	dev = cl->dev;

	if (!mei_cl_is_connected(cl))
		return -ENODEV;

	/* HW currently supports only one pending read */
	if (!list_empty(&cl->rd_pending) || mei_cl_is_read_fc_cb(cl))
		return -EBUSY;

	if (!mei_me_cl_is_active(cl->me_cl)) {
		cl_err(dev, cl, "no such me client\n");
		return  -ENOTTY;
	}

	/* always allocate at least client max message */
	length = max_t(size_t, length, mei_cl_mtu(cl));
	cb = mei_cl_alloc_cb(cl, length, MEI_FOP_READ, fp);
	if (!cb)
		return -ENOMEM;

	if (mei_cl_is_fixed_address(cl)) {
		list_add_tail(&cb->list, &cl->rd_pending);
		return 0;
	}

	rets = pm_runtime_get(dev->dev);
	if (rets < 0 && rets != -EINPROGRESS) {
		pm_runtime_put_noidle(dev->dev);
		cl_err(dev, cl, "rpm: get failed %d\n", rets);
		goto nortpm;
	}

	if (mei_hbuf_acquire(dev)) {
		rets = mei_hbm_cl_flow_control_req(dev, cl);
		if (rets < 0)
			goto out;

		list_add_tail(&cb->list, &cl->rd_pending);
	} else {
		rets = 0;
		list_add_tail(&cb->list, &dev->ctrl_wr_list.list);
	}

out:
	cl_dbg(dev, cl, "rpm: autosuspend\n");
	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_autosuspend(dev->dev);
nortpm:
	if (rets)
		mei_io_cb_free(cb);

	return rets;
}
