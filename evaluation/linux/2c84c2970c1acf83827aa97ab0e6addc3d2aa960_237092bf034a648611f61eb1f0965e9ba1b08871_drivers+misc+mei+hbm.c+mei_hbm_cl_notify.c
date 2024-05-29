static void mei_hbm_cl_notify(struct mei_device *dev,
			      struct mei_hbm_cl_cmd *cmd)
{
	struct mei_cl *cl;

	cl = mei_hbm_cl_find_by_cmd(dev, cmd);
	if (cl)
		mei_cl_notify(cl);
}
