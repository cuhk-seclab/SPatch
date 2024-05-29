int core_tpg_register(
	struct target_core_fabric_ops *tfo,
	struct se_wwn *se_wwn,
	struct se_portal_group *se_tpg,
	void *tpg_fabric_ptr,
	int se_tpg_type)
{
	struct se_lun *lun;
	u32 i;

	se_tpg->tpg_lun_list = array_zalloc(TRANSPORT_MAX_LUNS_PER_TPG,
			sizeof(struct se_lun), GFP_KERNEL);
	if (!se_tpg->tpg_lun_list) {
		pr_err("Unable to allocate struct se_portal_group->"
				"tpg_lun_list\n");
		return -ENOMEM;
	}

	for (i = 0; i < TRANSPORT_MAX_LUNS_PER_TPG; i++) {
		lun = se_tpg->tpg_lun_list[i];
		lun->unpacked_lun = i;
		lun->lun_link_magic = SE_LUN_LINK_MAGIC;
		lun->lun_status = TRANSPORT_LUN_STATUS_FREE;
		atomic_set(&lun->lun_acl_count, 0);
		init_completion(&lun->lun_shutdown_comp);
		INIT_LIST_HEAD(&lun->lun_acl_list);
		spin_lock_init(&lun->lun_acl_lock);
		spin_lock_init(&lun->lun_sep_lock);
		init_completion(&lun->lun_ref_comp);
	}

	se_tpg->se_tpg_type = se_tpg_type;
	se_tpg->se_tpg_fabric_ptr = tpg_fabric_ptr;
	se_tpg->se_tpg_tfo = tfo;
	se_tpg->se_tpg_wwn = se_wwn;
	atomic_set(&se_tpg->tpg_pr_ref_count, 0);
	INIT_LIST_HEAD(&se_tpg->acl_node_list);
	INIT_LIST_HEAD(&se_tpg->se_tpg_node);
	INIT_LIST_HEAD(&se_tpg->tpg_sess_list);
	spin_lock_init(&se_tpg->acl_node_lock);
	spin_lock_init(&se_tpg->session_lock);
	spin_lock_init(&se_tpg->tpg_lun_lock);

	if (se_tpg->se_tpg_type == TRANSPORT_TPG_TYPE_NORMAL) {
		if (core_tpg_setup_virtual_lun0(se_tpg) < 0) {
			array_free(se_tpg->tpg_lun_list,
				   TRANSPORT_MAX_LUNS_PER_TPG);
			return -ENOMEM;
		}
	}

	spin_lock_bh(&tpg_lock);
	list_add_tail(&se_tpg->se_tpg_node, &tpg_list);
	spin_unlock_bh(&tpg_lock);

	pr_debug("TARGET_CORE[%s]: Allocated %s struct se_portal_group for"
		" endpoint: %s, Portal Tag: %u\n", tfo->get_fabric_name(),
		(se_tpg->se_tpg_type == TRANSPORT_TPG_TYPE_NORMAL) ?
		"Normal" : "Discovery", (tfo->tpg_get_wwn(se_tpg) == NULL) ?
		"None" : tfo->tpg_get_wwn(se_tpg), tfo->tpg_get_tag(se_tpg));

	return 0;
}
