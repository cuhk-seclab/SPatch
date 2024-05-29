static sense_reason_t
core_scsi3_decode_spec_i_port(
	struct se_cmd *cmd,
	struct se_portal_group *tpg,
	unsigned char *l_isid,
	u64 sa_res_key,
	int all_tg_pt,
	int aptpl)
{
	struct se_device *dev = cmd->se_dev;
	struct se_port *tmp_port;
	struct se_portal_group *dest_tpg = NULL, *tmp_tpg;
	struct se_session *se_sess = cmd->se_sess;
	struct se_node_acl *dest_node_acl = NULL;
	struct se_dev_entry *dest_se_deve = NULL, *local_se_deve;
	struct t10_pr_registration *dest_pr_reg, *local_pr_reg, *pr_reg_e;
	struct t10_pr_registration *pr_reg_tmp, *pr_reg_tmp_safe;
	LIST_HEAD(tid_dest_list);
	struct pr_transport_id_holder *tidh_new, *tidh, *tidh_tmp;
	const struct target_core_fabric_ops *tmp_tf_ops;
	unsigned char *buf;
	unsigned char *ptr, *i_str = NULL, proto_ident;
	char *iport_ptr = NULL, i_buf[PR_REG_ISID_ID_LEN];
	sense_reason_t ret;
	u32 tpdl, tid_len = 0;
	int dest_local_nexus;
	u32 dest_rtpi = 0;

	local_se_deve = se_sess->se_node_acl->device_list[cmd->orig_fe_lun];
	/*
	 * Allocate a struct pr_transport_id_holder and setup the
	 * local_node_acl and local_se_deve pointers and add to
	 * struct list_head tid_dest_list for add registration
	 * processing in the loop of tid_dest_list below.
	 */
	tidh_new = kzalloc(sizeof(struct pr_transport_id_holder), GFP_KERNEL);
	if (!tidh_new) {
		pr_err("Unable to allocate tidh_new\n");
		return TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
	}
	INIT_LIST_HEAD(&tidh_new->dest_list);
	tidh_new->dest_tpg = tpg;
	tidh_new->dest_node_acl = se_sess->se_node_acl;
	tidh_new->dest_se_deve = local_se_deve;

	local_pr_reg = __core_scsi3_alloc_registration(cmd->se_dev,
				se_sess->se_node_acl, local_se_deve, l_isid,
				sa_res_key, all_tg_pt, aptpl);
	if (!local_pr_reg) {
		kfree(tidh_new);
		return TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
	}
	tidh_new->dest_pr_reg = local_pr_reg;
	/*
	 * The local I_T nexus does not hold any configfs dependances,
	 * so we set tid_h->dest_local_nexus=1 to prevent the
	 * configfs_undepend_item() calls in the tid_dest_list loops below.
	 */
	tidh_new->dest_local_nexus = 1;
	list_add_tail(&tidh_new->dest_list, &tid_dest_list);

	if (cmd->data_length < 28) {
		pr_warn("SPC-PR: Received PR OUT parameter list"
			" length too small: %u\n", cmd->data_length);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out;
	}

	buf = transport_kmap_data_sg(cmd);
	if (!buf) {
		ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		goto out;
	}

	/*
	 * For a PERSISTENT RESERVE OUT specify initiator ports payload,
	 * first extract TransportID Parameter Data Length, and make sure
	 * the value matches up to the SCSI expected data transfer length.
	 */
	tpdl = (buf[24] & 0xff) << 24;
	tpdl |= (buf[25] & 0xff) << 16;
	tpdl |= (buf[26] & 0xff) << 8;
	tpdl |= buf[27] & 0xff;

	if ((tpdl + 28) != cmd->data_length) {
		pr_err("SPC-3 PR: Illegal tpdl: %u + 28 byte header"
			" does not equal CDB data_length: %u\n", tpdl,
			cmd->data_length);
		ret = TCM_INVALID_PARAMETER_LIST;
		goto out_unmap;
	}
	/*
	 * Start processing the received transport IDs using the
	 * receiving I_T Nexus portal's fabric dependent methods to
	 * obtain the SCSI Initiator Port/Device Identifiers.
	 */
	ptr = &buf[28];

	while (tpdl > 0) {
		proto_ident = (ptr[0] & 0x0f);
		dest_tpg = NULL;

		spin_lock(&dev->se_port_lock);
		list_for_each_entry(tmp_port, &dev->dev_sep_list, sep_list) {
			tmp_tpg = tmp_port->sep_tpg;
			if (!tmp_tpg)
				continue;
			tmp_tf_ops = tmp_tpg->se_tpg_tfo;
			if (!tmp_tf_ops)
				continue;
			if (!tmp_tf_ops->tpg_parse_pr_out_transport_id)
				continue;
			/*
			 * Look for the matching proto_ident provided by
			 * the received TransportID
			 */
			if (tmp_tpg->proto_id != proto_ident)
				continue;
			dest_rtpi = tmp_port->sep_rtpi;

			i_str = tmp_tf_ops->tpg_parse_pr_out_transport_id(
					tmp_tpg, (const char *)ptr, &tid_len,
					&iport_ptr);
			if (!i_str)
				continue;

			atomic_inc_mb(&tmp_tpg->tpg_pr_ref_count);
			spin_unlock(&dev->se_port_lock);

			if (core_scsi3_tpg_depend_item(tmp_tpg)) {
				pr_err(" core_scsi3_tpg_depend_item()"
					" for tmp_tpg\n");
				atomic_dec_mb(&tmp_tpg->tpg_pr_ref_count);
				ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
				goto out_unmap;
			}
			/*
			 * Locate the destination initiator ACL to be registered
			 * from the decoded fabric module specific TransportID
			 * at *i_str.
			 */
			spin_lock_irq(&tmp_tpg->acl_node_lock);
			dest_node_acl = __core_tpg_get_initiator_node_acl(
						tmp_tpg, i_str);
			if (dest_node_acl)
				atomic_inc_mb(&dest_node_acl->acl_pr_ref_count);
			spin_unlock_irq(&tmp_tpg->acl_node_lock);

			if (!dest_node_acl) {
				core_scsi3_tpg_undepend_item(tmp_tpg);
				spin_lock(&dev->se_port_lock);
				continue;
			}

			if (core_scsi3_nodeacl_depend_item(dest_node_acl)) {
				pr_err("configfs_depend_item() failed"
					" for dest_node_acl->acl_group\n");
				atomic_dec_mb(&dest_node_acl->acl_pr_ref_count);
				core_scsi3_tpg_undepend_item(tmp_tpg);
				ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
				goto out_unmap;
			}

			dest_tpg = tmp_tpg;
			pr_debug("SPC-3 PR SPEC_I_PT: Located %s Node:"
				" %s Port RTPI: %hu\n",
				dest_tpg->se_tpg_tfo->get_fabric_name(),
				dest_node_acl->initiatorname, dest_rtpi);

			spin_lock(&dev->se_port_lock);
			break;
		}
		spin_unlock(&dev->se_port_lock);

		if (!dest_tpg) {
			pr_err("SPC-3 PR SPEC_I_PT: Unable to locate"
					" dest_tpg\n");
			ret = TCM_INVALID_PARAMETER_LIST;
			goto out_unmap;
		}

		pr_debug("SPC-3 PR SPEC_I_PT: Got %s data_length: %u tpdl: %u"
			" tid_len: %d for %s + %s\n",
			dest_tpg->se_tpg_tfo->get_fabric_name(), cmd->data_length,
			tpdl, tid_len, i_str, iport_ptr);

		if (tid_len > tpdl) {
			pr_err("SPC-3 PR SPEC_I_PT: Illegal tid_len:"
				" %u for Transport ID: %s\n", tid_len, ptr);
			core_scsi3_nodeacl_undepend_item(dest_node_acl);
			core_scsi3_tpg_undepend_item(dest_tpg);
			ret = TCM_INVALID_PARAMETER_LIST;
			goto out_unmap;
		}
		/*
		 * Locate the desintation struct se_dev_entry pointer for matching
		 * RELATIVE TARGET PORT IDENTIFIER on the receiving I_T Nexus
		 * Target Port.
		 */
		dest_se_deve = core_get_se_deve_from_rtpi(dest_node_acl,
					dest_rtpi);
		if (!dest_se_deve) {
			pr_err("Unable to locate %s dest_se_deve"
				" from destination RTPI: %hu\n",
				dest_tpg->se_tpg_tfo->get_fabric_name(),
				dest_rtpi);

			core_scsi3_nodeacl_undepend_item(dest_node_acl);
			core_scsi3_tpg_undepend_item(dest_tpg);
			ret = TCM_INVALID_PARAMETER_LIST;
			goto out_unmap;
		}

		if (core_scsi3_lunacl_depend_item(dest_se_deve)) {
			pr_err("core_scsi3_lunacl_depend_item()"
					" failed\n");
			atomic_dec_mb(&dest_se_deve->pr_ref_count);
			core_scsi3_nodeacl_undepend_item(dest_node_acl);
			core_scsi3_tpg_undepend_item(dest_tpg);
			ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			goto out_unmap;
		}

		pr_debug("SPC-3 PR SPEC_I_PT: Located %s Node: %s"
			" dest_se_deve mapped_lun: %u\n",
			dest_tpg->se_tpg_tfo->get_fabric_name(),
			dest_node_acl->initiatorname, dest_se_deve->mapped_lun);

		/*
		 * Skip any TransportIDs that already have a registration for
		 * this target port.
		 */
		pr_reg_e = __core_scsi3_locate_pr_reg(dev, dest_node_acl,
					iport_ptr);
		if (pr_reg_e) {
			core_scsi3_put_pr_reg(pr_reg_e);
			core_scsi3_lunacl_undepend_item(dest_se_deve);
			core_scsi3_nodeacl_undepend_item(dest_node_acl);
			core_scsi3_tpg_undepend_item(dest_tpg);
			ptr += tid_len;
			tpdl -= tid_len;
			tid_len = 0;
			continue;
		}
		/*
		 * Allocate a struct pr_transport_id_holder and setup
		 * the dest_node_acl and dest_se_deve pointers for the
		 * loop below.
		 */
		tidh_new = kzalloc(sizeof(struct pr_transport_id_holder),
				GFP_KERNEL);
		if (!tidh_new) {
			pr_err("Unable to allocate tidh_new\n");
			core_scsi3_lunacl_undepend_item(dest_se_deve);
			core_scsi3_nodeacl_undepend_item(dest_node_acl);
			core_scsi3_tpg_undepend_item(dest_tpg);
			ret = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			goto out_unmap;
		}
		INIT_LIST_HEAD(&tidh_new->dest_list);
		tidh_new->dest_tpg = dest_tpg;
		tidh_new->dest_node_acl = dest_node_acl;
		tidh_new->dest_se_deve = dest_se_deve;

		/*
		 * Allocate, but do NOT add the registration for the
		 * TransportID referenced SCSI Initiator port.  This
		 * done because of the following from spc4r17 in section
		 * 6.14.3 wrt SPEC_I_PT:
		 *
		 * "If a registration fails for any initiator port (e.g., if th
		 * logical unit does not have enough resources available to
		 * hold the registration information), no registrations shall be
		 * made, and the command shall be terminated with
		 * CHECK CONDITION status."
		 *
		 * That means we call __core_scsi3_alloc_registration() here,
		 * and then call __core_scsi3_add_registration() in the
		 * 2nd loop which will never fail.
		 */
		dest_pr_reg = __core_scsi3_alloc_registration(cmd->se_dev,
				dest_node_acl, dest_se_deve, iport_ptr,
				sa_res_key, all_tg_pt, aptpl);
		if (!dest_pr_reg) {
			core_scsi3_lunacl_undepend_item(dest_se_deve);
			core_scsi3_nodeacl_undepend_item(dest_node_acl);
			core_scsi3_tpg_undepend_item(dest_tpg);
			kfree(tidh_new);
			ret = TCM_INVALID_PARAMETER_LIST;
			goto out_unmap;
		}
		tidh_new->dest_pr_reg = dest_pr_reg;
		list_add_tail(&tidh_new->dest_list, &tid_dest_list);

		ptr += tid_len;
		tpdl -= tid_len;
		tid_len = 0;

	}

	transport_kunmap_data_sg(cmd);

	/*
	 * Go ahead and create a registrations from tid_dest_list for the
	 * SPEC_I_PT provided TransportID for the *tidh referenced dest_node_acl
	 * and dest_se_deve.
	 *
	 * The SA Reservation Key from the PROUT is set for the
	 * registration, and ALL_TG_PT is also passed.  ALL_TG_PT=1
	 * means that the TransportID Initiator port will be
	 * registered on all of the target ports in the SCSI target device
	 * ALL_TG_PT=0 means the registration will only be for the
	 * SCSI target port the PROUT REGISTER with SPEC_I_PT=1
	 * was received.
	 */
	list_for_each_entry_safe(tidh, tidh_tmp, &tid_dest_list, dest_list) {
		dest_tpg = tidh->dest_tpg;
		dest_node_acl = tidh->dest_node_acl;
		dest_se_deve = tidh->dest_se_deve;
		dest_pr_reg = tidh->dest_pr_reg;
		dest_local_nexus = tidh->dest_local_nexus;

		list_del(&tidh->dest_list);
		kfree(tidh);

		memset(i_buf, 0, PR_REG_ISID_ID_LEN);
		core_pr_dump_initiator_port(dest_pr_reg, i_buf, PR_REG_ISID_ID_LEN);

		__core_scsi3_add_registration(cmd->se_dev, dest_node_acl,
					dest_pr_reg, 0, 0);

		pr_debug("SPC-3 PR [%s] SPEC_I_PT: Successfully"
			" registered Transport ID for Node: %s%s Mapped LUN:"
			" %u\n", dest_tpg->se_tpg_tfo->get_fabric_name(),
			dest_node_acl->initiatorname, i_buf, dest_se_deve->mapped_lun);

		if (dest_local_nexus)
			continue;

		core_scsi3_lunacl_undepend_item(dest_se_deve);
		core_scsi3_nodeacl_undepend_item(dest_node_acl);
		core_scsi3_tpg_undepend_item(dest_tpg);
	}

	return 0;
out_unmap:
	transport_kunmap_data_sg(cmd);
out:
	/*
	 * For the failure case, release everything from tid_dest_list
	 * including *dest_pr_reg and the configfs dependances..
	 */
	list_for_each_entry_safe(tidh, tidh_tmp, &tid_dest_list, dest_list) {
		dest_tpg = tidh->dest_tpg;
		dest_node_acl = tidh->dest_node_acl;
		dest_se_deve = tidh->dest_se_deve;
		dest_pr_reg = tidh->dest_pr_reg;
		dest_local_nexus = tidh->dest_local_nexus;

		list_del(&tidh->dest_list);
		kfree(tidh);
		/*
		 * Release any extra ALL_TG_PT=1 registrations for
		 * the SPEC_I_PT=1 case.
		 */
		list_for_each_entry_safe(pr_reg_tmp, pr_reg_tmp_safe,
				&dest_pr_reg->pr_reg_atp_list,
				pr_reg_atp_mem_list) {
			list_del(&pr_reg_tmp->pr_reg_atp_mem_list);
			core_scsi3_lunacl_undepend_item(pr_reg_tmp->pr_reg_deve);
			kmem_cache_free(t10_pr_reg_cache, pr_reg_tmp);
		}

		kmem_cache_free(t10_pr_reg_cache, dest_pr_reg);

		if (dest_local_nexus)
			continue;

		core_scsi3_lunacl_undepend_item(dest_se_deve);
		core_scsi3_nodeacl_undepend_item(dest_node_acl);
		core_scsi3_tpg_undepend_item(dest_tpg);
	}
	return ret;
}
