static struct se_portal_group *ft_add_tpg(
	struct se_wwn *wwn,
	struct config_group *group,
	const char *name)
{
	struct ft_lport_wwn *ft_wwn;
	struct ft_tpg *tpg;
	struct workqueue_struct *wq;
	unsigned long index;
	int ret;

	pr_debug("tcm_fc: add tpg %s\n", name);

	/*
	 * Name must be "tpgt_" followed by the index.
	 */
	if (strstr(name, "tpgt_") != name)
		return NULL;

	ret = kstrtoul(name + 5, 10, &index);
	if (ret)
		return NULL;
	if (index > UINT_MAX)
		return NULL;

	if ((index != 1)) {
		pr_err("Error, a single TPG=1 is used for HW port mappings\n");
		return ERR_PTR(-ENOSYS);
	}

	ft_wwn = container_of(wwn, struct ft_lport_wwn, se_wwn);
	tpg = kzalloc(sizeof(*tpg), GFP_KERNEL);
	if (!tpg)
		return NULL;
	tpg->index = index;
	tpg->lport_wwn = ft_wwn;
	INIT_LIST_HEAD(&tpg->lun_list);

	wq = alloc_workqueue("tcm_fc", 0, 1);
	if (!wq) {
		kfree(tpg);
		return NULL;
	}

	ret = core_tpg_register(&ft_configfs->tf_ops, wwn, &tpg->se_tpg,
				tpg, TRANSPORT_TPG_TYPE_NORMAL);
	if (ret < 0) {
		destroy_workqueue(wq);
		kfree(tpg);
		return NULL;
	}
	tpg->workqueue = wq;

	mutex_lock(&ft_lport_lock);
	ft_wwn->tpg = tpg;
	mutex_unlock(&ft_lport_lock);

	return &tpg->se_tpg;
}
