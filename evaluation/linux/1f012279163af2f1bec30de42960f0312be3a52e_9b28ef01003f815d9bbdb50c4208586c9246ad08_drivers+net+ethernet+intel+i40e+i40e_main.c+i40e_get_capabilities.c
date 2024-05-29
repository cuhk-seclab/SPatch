static int i40e_get_capabilities(struct i40e_pf *pf)
{
	struct i40e_aqc_list_capabilities_element_resp *cap_buf;
	u16 data_size;
	int buf_len;
	int err;

	buf_len = 40 * sizeof(struct i40e_aqc_list_capabilities_element_resp);
	do {
		cap_buf = kzalloc(buf_len, GFP_KERNEL);
		if (!cap_buf)
			return -ENOMEM;

		/* this loads the data into the hw struct for us */
		err = i40e_aq_discover_capabilities(&pf->hw, cap_buf, buf_len,
					    &data_size,
					    i40e_aqc_opc_list_func_capabilities,
					    NULL);
		/* data loaded, buffer no longer needed */
		kfree(cap_buf);

		if (pf->hw.aq.asq_last_status == I40E_AQ_RC_ENOMEM) {
			/* retry with a larger buffer */
			buf_len = data_size;
		} else if (pf->hw.aq.asq_last_status != I40E_AQ_RC_OK) {
			dev_info(&pf->pdev->dev,
				 "capability discovery failed, err %s aq_err %s\n",
				 i40e_stat_str(&pf->hw, err),
				 i40e_aq_str(&pf->hw,
					     pf->hw.aq.asq_last_status));
			return -ENODEV;
		}
	} while (err);

	if (pf->hw.debug_mask & I40E_DEBUG_USER)
		dev_info(&pf->pdev->dev,
			 "pf=%d, num_vfs=%d, msix_pf=%d, msix_vf=%d, fd_g=%d, fd_b=%d, pf_max_q=%d num_vsi=%d\n",
			 pf->hw.pf_id, pf->hw.func_caps.num_vfs,
			 pf->hw.func_caps.num_msix_vectors,
			 pf->hw.func_caps.num_msix_vectors_vf,
			 pf->hw.func_caps.fd_filters_guaranteed,
			 pf->hw.func_caps.fd_filters_best_effort,
			 pf->hw.func_caps.num_tx_qp,
			 pf->hw.func_caps.num_vsis);

#define DEF_NUM_VSI (1 + (pf->hw.func_caps.fcoe ? 1 : 0) \
		       + pf->hw.func_caps.num_vfs)
	if (pf->hw.revision_id == 0 && (DEF_NUM_VSI > pf->hw.func_caps.num_vsis)) {
		dev_info(&pf->pdev->dev,
			 "got num_vsis %d, setting num_vsis to %d\n",
			 pf->hw.func_caps.num_vsis, DEF_NUM_VSI);
		pf->hw.func_caps.num_vsis = DEF_NUM_VSI;
	}

	return 0;
}
