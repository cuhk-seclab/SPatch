static int target_fabric_tf_ops_check(const struct target_core_fabric_ops *tfo)
{
	if (!tfo->name) {
		pr_err("Missing tfo->name\n");
		return -EINVAL;
	}
	if (strlen(tfo->name) >= TARGET_FABRIC_NAME_SIZE) {
		pr_err("Passed name: %s exceeds TARGET_FABRIC"
			"_NAME_SIZE\n", tfo->name);
		return -EINVAL;
	}
	if (!tfo->get_fabric_name) {
		pr_err("Missing tfo->get_fabric_name()\n");
		return -EINVAL;
	}
	if (!tfo->tpg_get_wwn) {
		pr_err("Missing tfo->tpg_get_wwn()\n");
		return -EINVAL;
	}
	if (!tfo->tpg_get_tag) {
		pr_err("Missing tfo->tpg_get_tag()\n");
		return -EINVAL;
	}
	if (!tfo->tpg_check_demo_mode) {
		pr_err("Missing tfo->tpg_check_demo_mode()\n");
		return -EINVAL;
	}
	if (!tfo->tpg_check_demo_mode_cache) {
		pr_err("Missing tfo->tpg_check_demo_mode_cache()\n");
		return -EINVAL;
	}
	if (!tfo->tpg_check_demo_mode_write_protect) {
		pr_err("Missing tfo->tpg_check_demo_mode_write_protect()\n");
		return -EINVAL;
	}
	if (!tfo->tpg_check_prod_mode_write_protect) {
		pr_err("Missing tfo->tpg_check_prod_mode_write_protect()\n");
		return -EINVAL;
	}
	if (!tfo->tpg_get_inst_index) {
		pr_err("Missing tfo->tpg_get_inst_index()\n");
		return -EINVAL;
	}
	if (!tfo->release_cmd) {
		pr_err("Missing tfo->release_cmd()\n");
		return -EINVAL;
	}
	if (!tfo->shutdown_session) {
		pr_err("Missing tfo->shutdown_session()\n");
		return -EINVAL;
	}
	if (!tfo->close_session) {
		pr_err("Missing tfo->close_session()\n");
		return -EINVAL;
	}
	if (!tfo->sess_get_index) {
		pr_err("Missing tfo->sess_get_index()\n");
		return -EINVAL;
	}
	if (!tfo->write_pending) {
		pr_err("Missing tfo->write_pending()\n");
		return -EINVAL;
	}
	if (!tfo->write_pending_status) {
		pr_err("Missing tfo->write_pending_status()\n");
		return -EINVAL;
	}
	if (!tfo->set_default_node_attributes) {
		pr_err("Missing tfo->set_default_node_attributes()\n");
		return -EINVAL;
	}
	if (!tfo->get_task_tag) {
		pr_err("Missing tfo->get_task_tag()\n");
		return -EINVAL;
	}
	if (!tfo->get_cmd_state) {
		pr_err("Missing tfo->get_cmd_state()\n");
		return -EINVAL;
	}
	if (!tfo->queue_data_in) {
		pr_err("Missing tfo->queue_data_in()\n");
		return -EINVAL;
	}
	if (!tfo->queue_status) {
		pr_err("Missing tfo->queue_status()\n");
		return -EINVAL;
	}
	if (!tfo->queue_tm_rsp) {
		pr_err("Missing tfo->queue_tm_rsp()\n");
		return -EINVAL;
	}
	if (!tfo->aborted_task) {
		pr_err("Missing tfo->aborted_task()\n");
		return -EINVAL;
	}
	/*
	 * We at least require tfo->fabric_make_wwn(), tfo->fabric_drop_wwn()
	 * tfo->fabric_make_tpg() and tfo->fabric_drop_tpg() in
	 * target_core_fabric_configfs.c WWN+TPG group context code.
	 */
	if (!tfo->fabric_make_wwn) {
		pr_err("Missing tfo->fabric_make_wwn()\n");
		return -EINVAL;
	}
	if (!tfo->fabric_drop_wwn) {
		pr_err("Missing tfo->fabric_drop_wwn()\n");
		return -EINVAL;
	}
	if (!tfo->fabric_make_tpg) {
		pr_err("Missing tfo->fabric_make_tpg()\n");
		return -EINVAL;
	}
	if (!tfo->fabric_drop_tpg) {
		pr_err("Missing tfo->fabric_drop_tpg()\n");
		return -EINVAL;
	}

	return 0;
}
