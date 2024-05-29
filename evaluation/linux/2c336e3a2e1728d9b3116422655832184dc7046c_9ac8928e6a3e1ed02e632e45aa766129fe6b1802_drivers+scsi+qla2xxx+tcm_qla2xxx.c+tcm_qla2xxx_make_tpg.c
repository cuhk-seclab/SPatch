static struct se_portal_group *tcm_qla2xxx_make_tpg(
	struct se_wwn *wwn,
	struct config_group *group,
	const char *name)
{
	struct tcm_qla2xxx_lport *lport = container_of(wwn,
			struct tcm_qla2xxx_lport, lport_wwn);
	struct tcm_qla2xxx_tpg *tpg;
	unsigned long tpgt;
	int ret;

	if (strstr(name, "tpgt_") != name)
		return ERR_PTR(-EINVAL);
	if (kstrtoul(name + 5, 10, &tpgt) || tpgt > USHRT_MAX)
		return ERR_PTR(-EINVAL);

	if ((tpgt != 1)) {
		pr_err("In non NPIV mode, a single TPG=1 is used for HW port mappings\n");
		return ERR_PTR(-ENOSYS);
	}

	tpg = kzalloc(sizeof(struct tcm_qla2xxx_tpg), GFP_KERNEL);
	if (!tpg) {
		pr_err("Unable to allocate struct tcm_qla2xxx_tpg\n");
		return ERR_PTR(-ENOMEM);
	}
	tpg->lport = lport;
	tpg->lport_tpgt = tpgt;
	/*
	 * By default allow READ-ONLY TPG demo-mode access w/ cached dynamic
	 * NodeACLs
	 */
	tpg->tpg_attrib.generate_node_acls = 1;
	tpg->tpg_attrib.demo_mode_write_protect = 1;
	tpg->tpg_attrib.cache_dynamic_acls = 1;
	tpg->tpg_attrib.demo_mode_login_only = 1;

	ret = core_tpg_register(&tcm_qla2xxx_fabric_configfs->tf_ops, wwn,
				&tpg->se_tpg, tpg, TRANSPORT_TPG_TYPE_NORMAL);
	if (ret < 0) {
		kfree(tpg);
		return NULL;
	}

	lport->tpg_1 = tpg;

	return &tpg->se_tpg;
}
