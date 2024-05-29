static void vxge_device_config_init(struct vxge_hw_device_config *device_config,
				    int *intr_type)
{
	/* Used for CQRQ/SRQ. */
	device_config->dma_blockpool_initial =
			VXGE_HW_INITIAL_DMA_BLOCK_POOL_SIZE;

	device_config->dma_blockpool_max =
			VXGE_HW_MAX_DMA_BLOCK_POOL_SIZE;

	if (max_mac_vpath > VXGE_MAX_MAC_ADDR_COUNT)
		max_mac_vpath = VXGE_MAX_MAC_ADDR_COUNT;

	if (!IS_ENABLED(CONFIG_PCI_MSI)) {
		vxge_debug_init(VXGE_ERR,
			"%s: This Kernel does not support "
			"MSI-X. Defaulting to INTA", VXGE_DRIVER_NAME);
		*intr_type = INTA;
	}

	/* Configure whether MSI-X or IRQL. */
	switch (*intr_type) {
	case INTA:
		device_config->intr_mode = VXGE_HW_INTR_MODE_IRQLINE;
		break;

	case MSI_X:
		device_config->intr_mode = VXGE_HW_INTR_MODE_MSIX_ONE_SHOT;
		break;
	}

	/* Timer period between device poll */
	device_config->device_poll_millis = VXGE_TIMER_DELAY;

	/* Configure mac based steering. */
	device_config->rts_mac_en = addr_learn_en;

	/* Configure Vpaths */
	device_config->rth_it_type = VXGE_HW_RTH_IT_TYPE_MULTI_IT;

	vxge_debug_ll_config(VXGE_TRACE, "%s : Device Config Params ",
			__func__);
	vxge_debug_ll_config(VXGE_TRACE, "intr_mode : %d",
			device_config->intr_mode);
	vxge_debug_ll_config(VXGE_TRACE, "device_poll_millis : %d",
			device_config->device_poll_millis);
	vxge_debug_ll_config(VXGE_TRACE, "rth_en : %d",
			device_config->rth_en);
	vxge_debug_ll_config(VXGE_TRACE, "rth_it_type : %d",
			device_config->rth_it_type);
}
