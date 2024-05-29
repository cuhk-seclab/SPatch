int iscsit_load_discovery_tpg(void)
{
	struct iscsi_param *param;
	struct iscsi_portal_group *tpg;
	int ret;

	tpg = iscsit_alloc_portal_group(NULL, 1);
	if (!tpg) {
		pr_err("Unable to allocate struct iscsi_portal_group\n");
		return -1;
	}

	ret = core_tpg_register(
			&lio_target_fabric_configfs->tf_ops,
			NULL, &tpg->tpg_se_tpg, tpg,
			TRANSPORT_TPG_TYPE_DISCOVERY);
	if (ret < 0) {
		kfree(tpg);
		return -1;
	}

	tpg->sid = 1; /* First Assigned LIO Session ID */
	iscsit_set_default_tpg_attribs(tpg);

	if (iscsi_create_default_params(&tpg->param_list) < 0)
		goto out;
	/*
	 * By default we disable authentication for discovery sessions,
	 * this can be changed with:
	 *
	 * /sys/kernel/config/target/iscsi/discovery_auth/enforce_discovery_auth
	 */
	param = iscsi_find_param_from_key(AUTHMETHOD, tpg->param_list);
	if (!param)
		goto out;

	if (iscsi_update_param_value(param, "CHAP,None") < 0)
		goto out;

	tpg->tpg_attrib.authentication = 0;

	spin_lock(&tpg->tpg_state_lock);
	tpg->tpg_state  = TPG_STATE_ACTIVE;
	spin_unlock(&tpg->tpg_state_lock);

	iscsit_global->discovery_tpg = tpg;
	pr_debug("CORE[0] - Allocated Discovery TPG\n");

	return 0;
out:
	if (tpg->sid == 1)
		core_tpg_deregister(&tpg->tpg_se_tpg);
	kfree(tpg);
	return -1;
}
