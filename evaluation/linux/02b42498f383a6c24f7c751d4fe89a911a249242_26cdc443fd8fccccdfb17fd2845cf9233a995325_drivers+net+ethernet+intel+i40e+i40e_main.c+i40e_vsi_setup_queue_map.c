static void i40e_vsi_setup_queue_map(struct i40e_vsi *vsi,
				     struct i40e_vsi_context *ctxt,
				     u8 enabled_tc,
				     bool is_add)
#endif
{
	struct i40e_pf *pf = vsi->back;
	u16 sections = 0;
	u8 netdev_tc = 0;
	u16 numtc = 0;
	u16 qcount;
	u8 offset;
	u16 qmap;
	int i;
	u16 num_tc_qps = 0;

	sections = I40E_AQ_VSI_PROP_QUEUE_MAP_VALID;
	offset = 0;

	if (enabled_tc && (vsi->back->flags & I40E_FLAG_DCB_ENABLED)) {
		/* Find numtc from enabled TC bitmap */
		for (i = 0; i < I40E_MAX_TRAFFIC_CLASS; i++) {
			if (enabled_tc & BIT_ULL(i)) /* TC is enabled */
				numtc++;
		}
		if (!numtc) {
			dev_warn(&pf->pdev->dev, "DCB is enabled but no TC enabled, forcing TC0\n");
			numtc = 1;
		}
	} else {
		/* At least TC0 is enabled in case of non-DCB case */
		numtc = 1;
	}

	vsi->tc_config.numtc = numtc;
	vsi->tc_config.enabled_tc = enabled_tc ? enabled_tc : 1;
	/* Number of queues per enabled TC */
	/* In MFP case we can have a much lower count of MSIx
	 * vectors available and so we need to lower the used
	 * q count.
	 */
	if (pf->flags & I40E_FLAG_MSIX_ENABLED)
		qcount = min_t(int, vsi->alloc_queue_pairs, pf->num_lan_msix);
	else
		qcount = vsi->alloc_queue_pairs;
	num_tc_qps = qcount / numtc;
	num_tc_qps = min_t(int, num_tc_qps, i40e_pf_get_max_q_per_tc(pf));

	/* Setup queue offset/count for all TCs for given VSI */
	for (i = 0; i < I40E_MAX_TRAFFIC_CLASS; i++) {
		/* See if the given TC is enabled for the given VSI */
		if (vsi->tc_config.enabled_tc & BIT_ULL(i)) {
			/* TC is enabled */
			int pow, num_qps;

			switch (vsi->type) {
			case I40E_VSI_MAIN:
				qcount = min_t(int, pf->rss_size, num_tc_qps);
				break;
#ifdef I40E_FCOE
			case I40E_VSI_FCOE:
				qcount = num_tc_qps;
				break;
#endif
			case I40E_VSI_FDIR:
			case I40E_VSI_SRIOV:
			case I40E_VSI_VMDQ2:
			default:
				qcount = num_tc_qps;
				WARN_ON(i != 0);
				break;
			}
			vsi->tc_config.tc_info[i].qoffset = offset;
			vsi->tc_config.tc_info[i].qcount = qcount;

			/* find the next higher power-of-2 of num queue pairs */
			num_qps = qcount;
			pow = 0;
			while (num_qps && (BIT_ULL(pow) < qcount)) {
				pow++;
				num_qps >>= 1;
			}

			vsi->tc_config.tc_info[i].netdev_tc = netdev_tc++;
			qmap =
			    (offset << I40E_AQ_VSI_TC_QUE_OFFSET_SHIFT) |
			    (pow << I40E_AQ_VSI_TC_QUE_NUMBER_SHIFT);

			offset += qcount;
		} else {
			/* TC is not enabled so set the offset to
			 * default queue and allocate one queue
			 * for the given TC.
			 */
			vsi->tc_config.tc_info[i].qoffset = 0;
			vsi->tc_config.tc_info[i].qcount = 1;
			vsi->tc_config.tc_info[i].netdev_tc = 0;

			qmap = 0;
		}
		ctxt->info.tc_mapping[i] = cpu_to_le16(qmap);
	}

	/* Set actual Tx/Rx queue pairs */
	vsi->num_queue_pairs = offset;
	if ((vsi->type == I40E_VSI_MAIN) && (numtc == 1)) {
		if (vsi->req_queue_pairs > 0)
			vsi->num_queue_pairs = vsi->req_queue_pairs;
		else if (pf->flags & I40E_FLAG_MSIX_ENABLED)
			vsi->num_queue_pairs = pf->num_lan_msix;
	}

	/* Scheduler section valid can only be set for ADD VSI */
	if (is_add) {
		sections |= I40E_AQ_VSI_PROP_SCHED_VALID;

		ctxt->info.up_enable_bits = enabled_tc;
	}
	if (vsi->type == I40E_VSI_SRIOV) {
		ctxt->info.mapping_flags |=
				     cpu_to_le16(I40E_AQ_VSI_QUE_MAP_NONCONTIG);
		for (i = 0; i < vsi->num_queue_pairs; i++)
			ctxt->info.queue_mapping[i] =
					       cpu_to_le16(vsi->base_queue + i);
	} else {
		ctxt->info.mapping_flags |=
					cpu_to_le16(I40E_AQ_VSI_QUE_MAP_CONTIG);
		ctxt->info.queue_mapping[0] = cpu_to_le16(vsi->base_queue);
	}
	ctxt->info.valid_sections |= cpu_to_le16(sections);
}
