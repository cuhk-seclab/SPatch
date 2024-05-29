static sense_reason_t
core_scsi3_emulate_pro_register_and_move(struct se_cmd *cmd, u64 res_key,
		u64 sa_res_key, int aptpl, int unreg)
{
	struct se_session *se_sess = cmd->se_sess;
	struct se_device *dev = cmd->se_dev;
	struct se_dev_entry *dest_se_deve = NULL;
	struct se_lun *se_lun = cmd->se_lun;
	struct se_node_acl *pr_res_nacl, *pr_reg_nacl, *dest_node_acl = NULL;
	struct se_port *se_port;
	struct se_portal_group *se_tpg, *dest_se_tpg = NULL;
	const struct target_core_fabric_ops *dest_tf_ops = NULL, *tf_ops;
	struct t10_pr_registration *pr_reg, *pr_res_holder, *dest_pr_reg;
	struct t10_reservation *pr_tmpl = &dev->t10_pr;
	unsigned char *buf;
	unsigned char *initiator_str;
	char *iport_ptr = NULL, i_buf[PR_REG_ISID_ID_LEN];
	u32 tid_len, tmp_tid_len;
	int new_reg = 0, type, scope, matching_iname;
	sense_reason_t ret;
	unsigned short rtpi;
	unsigned char proto_ident;

	if (!se_sess || !se_lun) {
		pr_err("SPC-3 PR: se_sess || struct se_lun is NULL!\n");
		return TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
	}

	memset(i_buf, 0, PR_REG_ISID_ID_LEN);
	se_tpg = se_sess->se_tpg;
	tf_ops = se_tpg->se_tpg_tfo;
	/*
	 * Follow logic from spc4r17 Section 5.7.8, Table 50 --
	 *	Register behaviors for a REGISTER AND MOVE service action
	 *
	 * Locate the existing *pr_reg via struct se_node_acl pointers
	 */
	pr_reg = core_scsi3_locate_pr_reg(cmd->se_dev, se_sess->se_node_acl,
				se_sess);
	if (!pr_reg) {
		pr_err("SPC-3 PR: Unable to locate PR_REGISTERED"
			" *pr_reg for REGISTER_AND_MOVE\n");
		return TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
	}
	/*
	 * The provided reservation key much match the existing reservation key
	 * provided during this initiator's I_T nexus registration.
	 */
	if (res_key != pr_reg->pr_res_key) {
		pr_warn("SPC-3 PR REGISTER_AND_MOVE: Received"
			" res_key: 0x%016Lx does not match existing SA REGISTER"
			" res_key: 0x%016Lx\n", res_key, pr_reg->pr_res_key);
		ret = TCM_RESERVATION_CONFLICT;
		goto out_put_pr_reg;
	}
	/*
	 * The service active reservation key needs to be non zero
	 */
	if (!sa_res_key) {
		pr_warn("SPC-3 PR REGISTER_AND_MOVE: Received zero"
			" sa_res_key\n");
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out_put_pr_reg;
	}

	/*
	 * Determine the Relative Target Port Identifier where the reservation
	 * will be moved to for the TransportID containing SCSI initiator WWN
	 * information.
	 */
	buf = transport_kmap_data_sg(cmd);
	if (!buf) {
		ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		goto out_put_pr_reg;
	}

	rtpi = (buf[18] & 0xff) << 8;
	rtpi |= buf[19] & 0xff;
	tid_len = (buf[20] & 0xff) << 24;
	tid_len |= (buf[21] & 0xff) << 16;
	tid_len |= (buf[22] & 0xff) << 8;
	tid_len |= buf[23] & 0xff;
	transport_kunmap_data_sg(cmd);
	buf = NULL;

	if ((tid_len + 24) != cmd->data_length) {
		pr_err("SPC-3 PR: Illegal tid_len: %u + 24 byte header"
			" does not equal CDB data_length: %u\n", tid_len,
			cmd->data_length);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out_put_pr_reg;
	}

	spin_lock(&dev->se_port_lock);
	list_for_each_entry(se_port, &dev->dev_sep_list, sep_list) {
		if (se_port->sep_rtpi != rtpi)
			continue;
		dest_se_tpg = se_port->sep_tpg;
		if (!dest_se_tpg)
			continue;
		dest_tf_ops = dest_se_tpg->se_tpg_tfo;
		if (!dest_tf_ops)
			continue;

		atomic_inc_mb(&dest_se_tpg->tpg_pr_ref_count);
		spin_unlock(&dev->se_port_lock);

		if (core_scsi3_tpg_depend_item(dest_se_tpg)) {
			pr_err("core_scsi3_tpg_depend_item() failed"
				" for dest_se_tpg\n");
			atomic_dec_mb(&dest_se_tpg->tpg_pr_ref_count);
			ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			goto out_put_pr_reg;
		}

		spin_lock(&dev->se_port_lock);
		break;
	}
	spin_unlock(&dev->se_port_lock);

	if (!dest_se_tpg || !dest_tf_ops) {
		pr_err("SPC-3 PR REGISTER_AND_MOVE: Unable to locate"
			" fabric ops from Relative Target Port Identifier:"
			" %hu\n", rtpi);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out_put_pr_reg;
	}

	buf = transport_kmap_data_sg(cmd);
	if (!buf) {
		ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		goto out_put_pr_reg;
	}
	proto_ident = (buf[24] & 0x0f);

	pr_debug("SPC-3 PR REGISTER_AND_MOVE: Extracted Protocol Identifier:"
			" 0x%02x\n", proto_ident);

	if (proto_ident != dest_tf_ops->get_fabric_proto_ident(dest_se_tpg)) {
		pr_err("SPC-3 PR REGISTER_AND_MOVE: Received"
			" proto_ident: 0x%02x does not match ident: 0x%02x"
			" from fabric: %s\n", proto_ident,
			dest_tf_ops->get_fabric_proto_ident(dest_se_tpg),
			dest_tf_ops->get_fabric_name());
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}
	if (dest_tf_ops->tpg_parse_pr_out_transport_id == NULL) {
		pr_err("SPC-3 PR REGISTER_AND_MOVE: Fabric does not"
			" containg a valid tpg_parse_pr_out_transport_id"
			" function pointer\n");
		ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		goto out;
	}
	initiator_str = dest_tf_ops->tpg_parse_pr_out_transport_id(dest_se_tpg,
			(const char *)&buf[24], &tmp_tid_len, &iport_ptr);
	if (!initiator_str) {
		pr_err("SPC-3 PR REGISTER_AND_MOVE: Unable to locate"
			" initiator_str from Transport ID\n");
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}

	transport_kunmap_data_sg(cmd);
	buf = NULL;

	pr_debug("SPC-3 PR [%s] Extracted initiator %s identifier: %s"
		" %s\n", dest_tf_ops->get_fabric_name(), (iport_ptr != NULL) ?
		"port" : "device", initiator_str, (iport_ptr != NULL) ?
		iport_ptr : "");
	/*
	 * If a PERSISTENT RESERVE OUT command with a REGISTER AND MOVE service
	 * action specifies a TransportID that is the same as the initiator port
	 * of the I_T nexus for the command received, then the command shall
	 * be terminated with CHECK CONDITION status, with the sense key set to
	 * ILLEGAL REQUEST, and the additional sense code set to INVALID FIELD
	 * IN PARAMETER LIST.
	 */
	pr_reg_nacl = pr_reg->pr_reg_nacl;
	matching_iname = (!strcmp(initiator_str,
				  pr_reg_nacl->initiatorname)) ? 1 : 0;
	if (!matching_iname)
		goto after_iport_check;

	if (!iport_ptr || !pr_reg->isid_present_at_reg) {
		pr_err("SPC-3 PR REGISTER_AND_MOVE: TransportID: %s"
			" matches: %s on received I_T Nexus\n", initiator_str,
			pr_reg_nacl->initiatorname);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}
	if (!strcmp(iport_ptr, pr_reg->pr_reg_isid)) {
		pr_err("SPC-3 PR REGISTER_AND_MOVE: TransportID: %s %s"
			" matches: %s %s on received I_T Nexus\n",
			initiator_str, iport_ptr, pr_reg_nacl->initiatorname,
			pr_reg->pr_reg_isid);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}
after_iport_check:
	/*
	 * Locate the destination struct se_node_acl from the received Transport ID
	 */
	spin_lock_irq(&dest_se_tpg->acl_node_lock);
	dest_node_acl = __core_tpg_get_initiator_node_acl(dest_se_tpg,
				initiator_str);
	if (dest_node_acl)
		atomic_inc_mb(&dest_node_acl->acl_pr_ref_count);
	spin_unlock_irq(&dest_se_tpg->acl_node_lock);

	if (!dest_node_acl) {
		pr_err("Unable to locate %s dest_node_acl for"
			" TransportID%s\n", dest_tf_ops->get_fabric_name(),
			initiator_str);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}

	if (core_scsi3_nodeacl_depend_item(dest_node_acl)) {
		pr_err("core_scsi3_nodeacl_depend_item() for"
			" dest_node_acl\n");
		atomic_dec_mb(&dest_node_acl->acl_pr_ref_count);
		dest_node_acl = NULL;
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}

	pr_debug("SPC-3 PR REGISTER_AND_MOVE: Found %s dest_node_acl:"
		" %s from TransportID\n", dest_tf_ops->get_fabric_name(),
		dest_node_acl->initiatorname);

	/*
	 * Locate the struct se_dev_entry pointer for the matching RELATIVE TARGET
	 * PORT IDENTIFIER.
	 */
	dest_se_deve = core_get_se_deve_from_rtpi(dest_node_acl, rtpi);
	if (!dest_se_deve) {
		pr_err("Unable to locate %s dest_se_deve from RTPI:"
			" %hu\n",  dest_tf_ops->get_fabric_name(), rtpi);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}

	if (core_scsi3_lunacl_depend_item(dest_se_deve)) {
		pr_err("core_scsi3_lunacl_depend_item() failed\n");
		atomic_dec_mb(&dest_se_deve->pr_ref_count);
		dest_se_deve = NULL;
		ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		goto out;
	}

	pr_debug("SPC-3 PR REGISTER_AND_MOVE: Located %s node %s LUN"
		" ACL for dest_se_deve->mapped_lun: %u\n",
		dest_tf_ops->get_fabric_name(), dest_node_acl->initiatorname,
		dest_se_deve->mapped_lun);

	/*
	 * A persistent reservation needs to already existing in order to
	 * successfully complete the REGISTER_AND_MOVE service action..
	 */
	spin_lock(&dev->dev_reservation_lock);
	pr_res_holder = dev->dev_pr_res_holder;
	if (!pr_res_holder) {
		pr_warn("SPC-3 PR REGISTER_AND_MOVE: No reservation"
			" currently held\n");
		spin_unlock(&dev->dev_reservation_lock);
		ret = TCM_INVALID_CDB_FIELD;
		goto out;
	}
	/*
	 * The received on I_T Nexus must be the reservation holder.
	 *
	 * From spc4r17 section 5.7.8  Table 50 --
	 * 	Register behaviors for a REGISTER AND MOVE service action
	 */
	if (!is_reservation_holder(pr_res_holder, pr_reg)) {
		pr_warn("SPC-3 PR REGISTER_AND_MOVE: Calling I_T"
			" Nexus is not reservation holder\n");
		spin_unlock(&dev->dev_reservation_lock);
		ret = TCM_RESERVATION_CONFLICT;
		goto out;
	}
	/*
	 * From spc4r17 section 5.7.8: registering and moving reservation
	 *
	 * If a PERSISTENT RESERVE OUT command with a REGISTER AND MOVE service
	 * action is received and the established persistent reservation is a
	 * Write Exclusive - All Registrants type or Exclusive Access -
	 * All Registrants type reservation, then the command shall be completed
	 * with RESERVATION CONFLICT status.
	 */
	if ((pr_res_holder->pr_res_type == PR_TYPE_WRITE_EXCLUSIVE_ALLREG) ||
	    (pr_res_holder->pr_res_type == PR_TYPE_EXCLUSIVE_ACCESS_ALLREG)) {
		pr_warn("SPC-3 PR REGISTER_AND_MOVE: Unable to move"
			" reservation for type: %s\n",
			core_scsi3_pr_dump_type(pr_res_holder->pr_res_type));
		spin_unlock(&dev->dev_reservation_lock);
		ret = TCM_RESERVATION_CONFLICT;
		goto out;
	}
	pr_res_nacl = pr_res_holder->pr_reg_nacl;
	/*
	 * b) Ignore the contents of the (received) SCOPE and TYPE fields;
	 */
	type = pr_res_holder->pr_res_type;
	scope = pr_res_holder->pr_res_type;
	/*
	 * c) Associate the reservation key specified in the SERVICE ACTION
	 *    RESERVATION KEY field with the I_T nexus specified as the
	 *    destination of the register and move, where:
	 *    A) The I_T nexus is specified by the TransportID and the
	 *	 RELATIVE TARGET PORT IDENTIFIER field (see 6.14.4); and
	 *    B) Regardless of the TransportID format used, the association for
	 *       the initiator port is based on either the initiator port name
	 *       (see 3.1.71) on SCSI transport protocols where port names are
	 *       required or the initiator port identifier (see 3.1.70) on SCSI
	 *       transport protocols where port names are not required;
	 * d) Register the reservation key specified in the SERVICE ACTION
	 *    RESERVATION KEY field;
	 * e) Retain the reservation key specified in the SERVICE ACTION
	 *    RESERVATION KEY field and associated information;
	 *
	 * Also, It is not an error for a REGISTER AND MOVE service action to
	 * register an I_T nexus that is already registered with the same
	 * reservation key or a different reservation key.
	 */
	dest_pr_reg = __core_scsi3_locate_pr_reg(dev, dest_node_acl,
					iport_ptr);
	if (!dest_pr_reg) {
		if (core_scsi3_alloc_registration(cmd->se_dev,
				dest_node_acl, dest_se_deve, iport_ptr,
				sa_res_key, 0, aptpl, 2, 1)) {
			spin_unlock(&dev->dev_reservation_lock);
			ret = TCM_INVALID_PARAMETER_LIST;
			goto out;
		}
		dest_pr_reg = __core_scsi3_locate_pr_reg(dev, dest_node_acl,
						iport_ptr);
		new_reg = 1;
	}
	/*
	 * f) Release the persistent reservation for the persistent reservation
	 *    holder (i.e., the I_T nexus on which the
	 */
	__core_scsi3_complete_pro_release(dev, pr_res_nacl,
					  dev->dev_pr_res_holder, 0, 0);
	/*
	 * g) Move the persistent reservation to the specified I_T nexus using
	 *    the same scope and type as the persistent reservation released in
	 *    item f); and
	 */
	dev->dev_pr_res_holder = dest_pr_reg;
	dest_pr_reg->pr_res_holder = 1;
	dest_pr_reg->pr_res_type = type;
	pr_reg->pr_res_scope = scope;
	core_pr_dump_initiator_port(pr_reg, i_buf, PR_REG_ISID_ID_LEN);
	/*
	 * Increment PRGeneration for existing registrations..
	 */
	if (!new_reg)
		dest_pr_reg->pr_res_generation = pr_tmpl->pr_generation++;
	spin_unlock(&dev->dev_reservation_lock);

	pr_debug("SPC-3 PR [%s] Service Action: REGISTER_AND_MOVE"
		" created new reservation holder TYPE: %s on object RTPI:"
		" %hu  PRGeneration: 0x%08x\n", dest_tf_ops->get_fabric_name(),
		core_scsi3_pr_dump_type(type), rtpi,
		dest_pr_reg->pr_res_generation);
	pr_debug("SPC-3 PR Successfully moved reservation from"
		" %s Fabric Node: %s%s -> %s Fabric Node: %s %s\n",
		tf_ops->get_fabric_name(), pr_reg_nacl->initiatorname,
		i_buf, dest_tf_ops->get_fabric_name(),
		dest_node_acl->initiatorname, (iport_ptr != NULL) ?
		iport_ptr : "");
	/*
	 * It is now safe to release configfs group dependencies for destination
	 * of Transport ID Initiator Device/Port Identifier
	 */
	core_scsi3_lunacl_undepend_item(dest_se_deve);
	core_scsi3_nodeacl_undepend_item(dest_node_acl);
	core_scsi3_tpg_undepend_item(dest_se_tpg);
	/*
	 * h) If the UNREG bit is set to one, unregister (see 5.7.11.3) the I_T
	 * nexus on which PERSISTENT RESERVE OUT command was received.
	 */
	if (unreg) {
		spin_lock(&pr_tmpl->registration_lock);
		__core_scsi3_free_registration(dev, pr_reg, NULL, 1);
		spin_unlock(&pr_tmpl->registration_lock);
	} else
		core_scsi3_put_pr_reg(pr_reg);

	core_scsi3_update_and_write_aptpl(cmd->se_dev, aptpl);

	transport_kunmap_data_sg(cmd);

	core_scsi3_put_pr_reg(dest_pr_reg);
	return 0;
out:
	if (buf)
		transport_kunmap_data_sg(cmd);
	if (dest_se_deve)
		core_scsi3_lunacl_undepend_item(dest_se_deve);
	if (dest_node_acl)
		core_scsi3_nodeacl_undepend_item(dest_node_acl);
	core_scsi3_tpg_undepend_item(dest_se_tpg);

out_put_pr_reg:
	core_scsi3_put_pr_reg(pr_reg);
	return ret;
}
