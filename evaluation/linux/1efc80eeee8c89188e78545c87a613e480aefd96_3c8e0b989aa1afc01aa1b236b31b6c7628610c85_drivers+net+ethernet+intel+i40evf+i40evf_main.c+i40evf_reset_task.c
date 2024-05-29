static void i40evf_reset_task(struct work_struct *work)
{
	struct i40evf_adapter *adapter = container_of(work,
						      struct i40evf_adapter,
						      reset_task);
	struct net_device *netdev = adapter->netdev;
	struct i40e_hw *hw = &adapter->hw;
	struct i40evf_mac_filter *f;
	uint32_t rstat_val;
	int i = 0, err;

	while (test_and_set_bit(__I40EVF_IN_CRITICAL_TASK,
				&adapter->crit_section))
		usleep_range(500, 1000);

	if (adapter->flags & I40EVF_FLAG_RESET_NEEDED) {
		dev_info(&adapter->pdev->dev, "Requesting reset from PF\n");
		i40evf_request_reset(adapter);
	}

	/* poll until we see the reset actually happen */
	for (i = 0; i < I40EVF_RESET_WAIT_COUNT; i++) {
		rstat_val = rd32(hw, I40E_VFGEN_RSTAT) &
			    I40E_VFGEN_RSTAT_VFR_STATE_MASK;
		if ((rstat_val != I40E_VFR_VFACTIVE) &&
		    (rstat_val != I40E_VFR_COMPLETED))
			break;
		msleep(I40EVF_RESET_WAIT_MS);
	}
	if (i == I40EVF_RESET_WAIT_COUNT) {
		adapter->flags &= ~I40EVF_FLAG_RESET_PENDING;
		goto continue_reset; /* act like the reset happened */
	}

	/* wait until the reset is complete and the PF is responding to us */
	for (i = 0; i < I40EVF_RESET_WAIT_COUNT; i++) {
		rstat_val = rd32(hw, I40E_VFGEN_RSTAT) &
			    I40E_VFGEN_RSTAT_VFR_STATE_MASK;
		if ((rstat_val == I40E_VFR_VFACTIVE) ||
		    (rstat_val == I40E_VFR_COMPLETED))
			break;
		msleep(I40EVF_RESET_WAIT_MS);
	}
	if (i == I40EVF_RESET_WAIT_COUNT) {
		struct i40evf_mac_filter *f, *ftmp;
		struct i40evf_vlan_filter *fv, *fvtmp;

		/* reset never finished */
		dev_err(&adapter->pdev->dev, "Reset never finished (%x)\n",
			rstat_val);
		adapter->flags |= I40EVF_FLAG_PF_COMMS_FAILED;

		if (netif_running(adapter->netdev)) {
			set_bit(__I40E_DOWN, &adapter->vsi.state);
			i40evf_irq_disable(adapter);
			i40evf_napi_disable_all(adapter);
			netif_tx_disable(netdev);
			netif_tx_stop_all_queues(netdev);
			netif_carrier_off(netdev);
			i40evf_free_traffic_irqs(adapter);
			i40evf_free_all_tx_resources(adapter);
			i40evf_free_all_rx_resources(adapter);
		}

		/* Delete all of the filters, both MAC and VLAN. */
		list_for_each_entry_safe(f, ftmp, &adapter->mac_filter_list,
					 list) {
			list_del(&f->list);
			kfree(f);
		}
		list_for_each_entry_safe(fv, fvtmp, &adapter->vlan_filter_list,
					 list) {
			list_del(&fv->list);
			kfree(fv);
		}

		i40evf_free_misc_irq(adapter);
		i40evf_reset_interrupt_capability(adapter);
		i40evf_free_queues(adapter);
		i40evf_free_q_vectors(adapter);
		kfree(adapter->vf_res);
		i40evf_shutdown_adminq(hw);
		adapter->netdev->flags &= ~IFF_UP;
		clear_bit(__I40EVF_IN_CRITICAL_TASK, &adapter->crit_section);
		return; /* Do not attempt to reinit. It's dead, Jim. */
	}

continue_reset:
	adapter->flags &= ~I40EVF_FLAG_RESET_PENDING;

	i40evf_irq_disable(adapter);

	if (netif_running(adapter->netdev)) {
		i40evf_napi_disable_all(adapter);
		netif_tx_disable(netdev);
		netif_tx_stop_all_queues(netdev);
		netif_carrier_off(netdev);
	}

	adapter->state = __I40EVF_RESETTING;

	/* kill and reinit the admin queue */
	if (i40evf_shutdown_adminq(hw))
		dev_warn(&adapter->pdev->dev, "Failed to shut down adminq\n");
	adapter->current_op = I40E_VIRTCHNL_OP_UNKNOWN;
	err = i40evf_init_adminq(hw);
	if (err)
		dev_info(&adapter->pdev->dev, "Failed to init adminq: %d\n",
			 err);

	i40evf_map_queues(adapter);

	/* re-add all MAC filters */
	list_for_each_entry(f, &adapter->mac_filter_list, list) {
		f->add = true;
	}
	/* re-add all VLAN filters */
	list_for_each_entry(f, &adapter->vlan_filter_list, list) {
		f->add = true;
	}
	adapter->aq_required = I40EVF_FLAG_AQ_ADD_MAC_FILTER;
	adapter->aq_required |= I40EVF_FLAG_AQ_ADD_VLAN_FILTER;
	clear_bit(__I40EVF_IN_CRITICAL_TASK, &adapter->crit_section);

	mod_timer(&adapter->watchdog_timer, jiffies + 2);

	if (netif_running(adapter->netdev)) {
		/* allocate transmit descriptors */
		err = i40evf_setup_all_tx_resources(adapter);
		if (err)
			goto reset_err;

		/* allocate receive descriptors */
		err = i40evf_setup_all_rx_resources(adapter);
		if (err)
			goto reset_err;

		i40evf_configure(adapter);

		err = i40evf_up_complete(adapter);
		if (err)
			goto reset_err;

		i40evf_irq_enable(adapter, true);
	}
	return;
reset_err:
	dev_err(&adapter->pdev->dev, "failed to allocate resources during reinit\n");
	i40evf_close(adapter->netdev);
}
