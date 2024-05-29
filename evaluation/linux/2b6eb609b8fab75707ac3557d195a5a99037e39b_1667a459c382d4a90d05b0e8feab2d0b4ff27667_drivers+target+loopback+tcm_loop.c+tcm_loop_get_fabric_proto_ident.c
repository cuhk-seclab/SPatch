static u8 tcm_loop_get_fabric_proto_ident(struct se_portal_group *se_tpg)
{
	struct tcm_loop_tpg *tl_tpg = se_tpg->se_tpg_fabric_ptr;
	struct tcm_loop_hba *tl_hba = tl_tpg->tl_hba;
	/*
	 * tl_proto_id is set at tcm_loop_configfs.c:tcm_loop_make_scsi_hba()
	 * time based on the protocol dependent prefix of the passed configfs group.
	 *
	 * Based upon tl_proto_id, TCM_Loop emulates the requested fabric
	 * ProtocolID using target_core_fabric_lib.c symbols.
	 */
	switch (tl_hba->tl_proto_id) {
	case SCSI_PROTOCOL_SAS:
		return sas_get_fabric_proto_ident(se_tpg);
	case SCSI_PROTOCOL_FCP:
		return fc_get_fabric_proto_ident(se_tpg);
	case SCSI_PROTOCOL_ISCSI:
		return iscsi_get_fabric_proto_ident(se_tpg);
	default:
		pr_err("Unknown tl_proto_id: 0x%02x, using"
			" SAS emulation\n", tl_hba->tl_proto_id);
		break;
	}

	return sas_get_fabric_proto_ident(se_tpg);
}
