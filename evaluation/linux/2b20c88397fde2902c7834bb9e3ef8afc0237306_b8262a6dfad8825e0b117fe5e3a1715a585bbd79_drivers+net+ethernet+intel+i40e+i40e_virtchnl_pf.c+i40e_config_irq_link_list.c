static void i40e_config_irq_link_list(struct i40e_vf *vf, u16 vsi_id,
				      struct i40e_virtchnl_vector_map *vecmap)
{
	unsigned long linklistmap = 0, tempmap;
	struct i40e_pf *pf = vf->pf;
	struct i40e_hw *hw = &pf->hw;
	u16 vsi_queue_id, pf_queue_id;
	enum i40e_queue_type qtype;
	u16 next_q, vector_id;
	u32 reg, reg_idx;
	u16 itr_idx = 0;

	vector_id = vecmap->vector_id;
	/* setup the head */
	if (0 == vector_id)
		reg_idx = I40E_VPINT_LNKLST0(vf->vf_id);
	else
		reg_idx = I40E_VPINT_LNKLSTN(
		     ((pf->hw.func_caps.num_msix_vectors_vf - 1) * vf->vf_id) +
		     (vector_id - 1));

	if (vecmap->rxq_map == 0 && vecmap->txq_map == 0) {
		/* Special case - No queues mapped on this vector */
		wr32(hw, reg_idx, I40E_VPINT_LNKLST0_FIRSTQ_INDX_MASK);
		goto irq_list_done;
	}
	tempmap = vecmap->rxq_map;
	for_each_set_bit(vsi_queue_id, &tempmap, I40E_MAX_VSI_QP) {
		linklistmap |= (BIT(I40E_VIRTCHNL_SUPPORTED_QTYPES *
				    vsi_queue_id));
	}

	tempmap = vecmap->txq_map;
	for_each_set_bit(vsi_queue_id, &tempmap, I40E_MAX_VSI_QP) {
		linklistmap |= (BIT(I40E_VIRTCHNL_SUPPORTED_QTYPES *
				     vsi_queue_id + 1));
	}

	next_q = find_first_bit(&linklistmap,
				(I40E_MAX_VSI_QP *
				 I40E_VIRTCHNL_SUPPORTED_QTYPES));
	vsi_queue_id = next_q/I40E_VIRTCHNL_SUPPORTED_QTYPES;
	qtype = next_q%I40E_VIRTCHNL_SUPPORTED_QTYPES;
	pf_queue_id = i40e_vc_get_pf_queue_id(vf, vsi_id, vsi_queue_id);
	reg = ((qtype << I40E_VPINT_LNKLSTN_FIRSTQ_TYPE_SHIFT) | pf_queue_id);

	wr32(hw, reg_idx, reg);

	while (next_q < (I40E_MAX_VSI_QP * I40E_VIRTCHNL_SUPPORTED_QTYPES)) {
		switch (qtype) {
		case I40E_QUEUE_TYPE_RX:
			reg_idx = I40E_QINT_RQCTL(pf_queue_id);
			itr_idx = vecmap->rxitr_idx;
			break;
		case I40E_QUEUE_TYPE_TX:
			reg_idx = I40E_QINT_TQCTL(pf_queue_id);
			itr_idx = vecmap->txitr_idx;
			break;
		default:
			break;
		}

		next_q = find_next_bit(&linklistmap,
				       (I40E_MAX_VSI_QP *
					I40E_VIRTCHNL_SUPPORTED_QTYPES),
				       next_q + 1);
		if (next_q <
		    (I40E_MAX_VSI_QP * I40E_VIRTCHNL_SUPPORTED_QTYPES)) {
			vsi_queue_id = next_q / I40E_VIRTCHNL_SUPPORTED_QTYPES;
			qtype = next_q % I40E_VIRTCHNL_SUPPORTED_QTYPES;
			pf_queue_id = i40e_vc_get_pf_queue_id(vf, vsi_id,
							      vsi_queue_id);
		} else {
			pf_queue_id = I40E_QUEUE_END_OF_LIST;
			qtype = 0;
		}

		/* format for the RQCTL & TQCTL regs is same */
		reg = (vector_id) |
		    (qtype << I40E_QINT_RQCTL_NEXTQ_TYPE_SHIFT) |
		    (pf_queue_id << I40E_QINT_RQCTL_NEXTQ_INDX_SHIFT) |
		    BIT(I40E_QINT_RQCTL_CAUSE_ENA_SHIFT) |
		    (itr_idx << I40E_QINT_RQCTL_ITR_INDX_SHIFT);
		wr32(hw, reg_idx, reg);
	}

	/* if the vf is running in polling mode and using interrupt zero,
	 * need to disable auto-mask on enabling zero interrupt for VFs.
	 */
	if ((vf->driver_caps & I40E_VIRTCHNL_VF_OFFLOAD_RX_POLLING) &&
	    (vector_id == 0)) {
		reg = rd32(hw, I40E_GLINT_CTL);
		if (!(reg & I40E_GLINT_CTL_DIS_AUTOMASK_VF0_MASK)) {
			reg |= I40E_GLINT_CTL_DIS_AUTOMASK_VF0_MASK;
			wr32(hw, I40E_GLINT_CTL, reg);
		}
	}

irq_list_done:
	i40e_flush(hw);
}
