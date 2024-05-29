int i40e_reconfig_rss_queues(struct i40e_pf *pf, int queue_count)
{
	struct i40e_vsi *vsi = pf->vsi[pf->lan_vsi];
	int new_rss_size;

	if (!(pf->flags & I40E_FLAG_RSS_ENABLED))
		return 0;

	new_rss_size = min_t(int, queue_count, pf->rss_size_max);

	if (queue_count != vsi->num_queue_pairs) {
		vsi->req_queue_pairs = queue_count;
		i40e_prep_for_reset(pf);

		pf->alloc_rss_size = new_rss_size;

		i40e_reset_and_rebuild(pf, true);

		/* Discard the user configured hash keys and lut, if less
		 * queues are enabled.
		 */
		if (queue_count < vsi->rss_size) {
			i40e_clear_rss_config_user(vsi);
			dev_dbg(&pf->pdev->dev,
				"discard user configured hash keys and lut\n");
		}

		/* Reset vsi->rss_size, as number of enabled queues changed */
		vsi->rss_size = min_t(int, pf->alloc_rss_size,
				      vsi->num_queue_pairs);

		i40e_pf_config_rss(pf);
	}
	dev_info(&pf->pdev->dev, "RSS count/HW max RSS count:  %d/%d\n",
		 pf->alloc_rss_size, pf->rss_size_max);
	return pf->alloc_rss_size;
}
