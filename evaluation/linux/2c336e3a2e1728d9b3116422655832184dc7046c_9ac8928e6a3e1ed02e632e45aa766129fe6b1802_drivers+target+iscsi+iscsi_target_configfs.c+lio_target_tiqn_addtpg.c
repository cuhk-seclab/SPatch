static struct se_portal_group *lio_target_tiqn_addtpg(
	struct se_wwn *wwn,
	struct config_group *group,
	const char *name)
{
	struct iscsi_portal_group *tpg;
	struct iscsi_tiqn *tiqn;
	char *tpgt_str;
	int ret;
	u16 tpgt;

	tiqn = container_of(wwn, struct iscsi_tiqn, tiqn_wwn);
	/*
	 * Only tpgt_# directory groups can be created below
	 * target/iscsi/iqn.superturodiskarry/
	 */
	tpgt_str = strstr(name, "tpgt_");
	if (!tpgt_str) {
		pr_err("Unable to locate \"tpgt_#\" directory"
				" group\n");
		return NULL;
	}
	tpgt_str += 5; /* Skip ahead of "tpgt_" */
	ret = kstrtou16(tpgt_str, 0, &tpgt);
	if (ret)
		return NULL;

	tpg = iscsit_alloc_portal_group(tiqn, tpgt);
	if (!tpg)
		return NULL;

	ret = core_tpg_register(
			&lio_target_fabric_configfs->tf_ops,
			wwn, &tpg->tpg_se_tpg, tpg,
			TRANSPORT_TPG_TYPE_NORMAL);
	if (ret < 0)
		return NULL;

	ret = iscsit_tpg_add_portal_group(tiqn, tpg);
	if (ret != 0)
		goto out;

	pr_debug("LIO_Target_ConfigFS: REGISTER -> %s\n", tiqn->tiqn);
	pr_debug("LIO_Target_ConfigFS: REGISTER -> Allocated TPG: %s\n",
			name);
	return &tpg->tpg_se_tpg;
out:
	core_tpg_deregister(&tpg->tpg_se_tpg);
	kfree(tpg);
	return NULL;
}
