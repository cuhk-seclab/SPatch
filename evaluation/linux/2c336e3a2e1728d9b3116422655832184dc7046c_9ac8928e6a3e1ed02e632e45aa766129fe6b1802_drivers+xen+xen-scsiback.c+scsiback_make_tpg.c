static struct se_portal_group *
scsiback_make_tpg(struct se_wwn *wwn,
		   struct config_group *group,
		   const char *name)
{
	struct scsiback_tport *tport = container_of(wwn,
			struct scsiback_tport, tport_wwn);

	struct scsiback_tpg *tpg;
	u16 tpgt;
	int ret;

	if (strstr(name, "tpgt_") != name)
		return ERR_PTR(-EINVAL);
	ret = kstrtou16(name + 5, 10, &tpgt);
	if (ret)
		return ERR_PTR(ret);

	tpg = kzalloc(sizeof(struct scsiback_tpg), GFP_KERNEL);
	if (!tpg)
		return ERR_PTR(-ENOMEM);

	mutex_init(&tpg->tv_tpg_mutex);
	INIT_LIST_HEAD(&tpg->tv_tpg_list);
	INIT_LIST_HEAD(&tpg->info_list);
	tpg->tport = tport;
	tpg->tport_tpgt = tpgt;

	ret = core_tpg_register(&scsiback_fabric_configfs->tf_ops, wwn,
				&tpg->se_tpg, tpg, TRANSPORT_TPG_TYPE_NORMAL);
	if (ret < 0) {
		kfree(tpg);
		return NULL;
	}
	mutex_lock(&scsiback_mutex);
	list_add_tail(&tpg->tv_tpg_list, &scsiback_list);
	mutex_unlock(&scsiback_mutex);

	return &tpg->se_tpg;
}
