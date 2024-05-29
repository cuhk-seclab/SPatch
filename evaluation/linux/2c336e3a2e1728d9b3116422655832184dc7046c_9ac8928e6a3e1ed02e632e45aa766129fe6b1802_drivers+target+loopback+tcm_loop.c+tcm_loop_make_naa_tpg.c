static struct se_portal_group *tcm_loop_make_naa_tpg(
	struct se_wwn *wwn,
	struct config_group *group,
	const char *name)
{
	struct tcm_loop_hba *tl_hba = container_of(wwn,
			struct tcm_loop_hba, tl_hba_wwn);
	struct tcm_loop_tpg *tl_tpg;
	int ret;
	unsigned long tpgt;

	if (strstr(name, "tpgt_") != name) {
		pr_err("Unable to locate \"tpgt_#\" directory"
				" group\n");
		return ERR_PTR(-EINVAL);
	}
	if (kstrtoul(name+5, 10, &tpgt))
		return ERR_PTR(-EINVAL);

	if (tpgt >= TL_TPGS_PER_HBA) {
		pr_err("Passed tpgt: %lu exceeds TL_TPGS_PER_HBA:"
				" %u\n", tpgt, TL_TPGS_PER_HBA);
		return ERR_PTR(-EINVAL);
	}
	tl_tpg = &tl_hba->tl_hba_tpgs[tpgt];
	tl_tpg->tl_hba = tl_hba;
	tl_tpg->tl_tpgt = tpgt;
	/*
	 * Register the tl_tpg as a emulated SAS TCM Target Endpoint
	 */
	ret = core_tpg_register(&tcm_loop_fabric_configfs->tf_ops,
			wwn, &tl_tpg->tl_se_tpg, tl_tpg,
			TRANSPORT_TPG_TYPE_NORMAL);
	if (ret < 0)
		return ERR_PTR(-ENOMEM);

	pr_debug("TCM_Loop_ConfigFS: Allocated Emulated %s"
		" Target Port %s,t,0x%04lx\n", tcm_loop_dump_proto_id(tl_hba),
		config_item_name(&wwn->wwn_group.cg_item), tpgt);

	return &tl_tpg->tl_se_tpg;
}
