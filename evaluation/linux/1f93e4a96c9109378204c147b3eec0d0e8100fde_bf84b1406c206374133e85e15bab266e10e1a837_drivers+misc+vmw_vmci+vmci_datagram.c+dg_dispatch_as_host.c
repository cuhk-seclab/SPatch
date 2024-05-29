static int dg_dispatch_as_host(u32 context_id, struct vmci_datagram *dg)
{
	int retval;
	size_t dg_size;
	u32 src_priv_flags;

	dg_size = VMCI_DG_SIZE(dg);

	/* Host cannot send to the hypervisor. */
	if (dg->dst.context == VMCI_HYPERVISOR_CONTEXT_ID)
		return VMCI_ERROR_DST_UNREACHABLE;

	/* Check that source handle matches sending context. */
	if (dg->src.context != context_id) {
		pr_devel("Sender context (ID=0x%x) is not owner of src datagram entry (handle=0x%x:0x%x)\n",
			 context_id, dg->src.context, dg->src.resource);
		return VMCI_ERROR_NO_ACCESS;
	}

	/* Get hold of privileges of sending endpoint. */
	retval = vmci_datagram_get_priv_flags(context_id, dg->src,
					      &src_priv_flags);
	if (retval != VMCI_SUCCESS) {
		pr_warn("Couldn't get privileges (handle=0x%x:0x%x)\n",
			dg->src.context, dg->src.resource);
		return retval;
	}

	/* Determine if we should route to host or guest destination. */
	if (dg->dst.context == VMCI_HOST_CONTEXT_ID) {
		/* Route to host datagram entry. */
		struct datagram_entry *dst_entry;
		struct vmci_resource *resource;

		if (dg->src.context == VMCI_HYPERVISOR_CONTEXT_ID &&
		    dg->dst.resource == VMCI_EVENT_HANDLER) {
			return vmci_event_dispatch(dg);
		}

		resource = vmci_resource_by_handle(dg->dst,
						   VMCI_RESOURCE_TYPE_DATAGRAM);
		if (!resource) {
			pr_devel("Sending to invalid destination (handle=0x%x:0x%x)\n",
				 dg->dst.context, dg->dst.resource);
			return VMCI_ERROR_INVALID_RESOURCE;
		}
		dst_entry = container_of(resource, struct datagram_entry,
					 resource);
		if (vmci_deny_interaction(src_priv_flags,
					  dst_entry->priv_flags)) {
			vmci_resource_put(resource);
			return VMCI_ERROR_NO_ACCESS;
		}

		/*
		 * If a VMCI datagram destined for the host is also sent by the
		 * host, we always run it delayed. This ensures that no locks
		 * are held when the datagram callback runs.
		 */
		if (dst_entry->run_delayed ||
		    dg->src.context == VMCI_HOST_CONTEXT_ID) {
			struct delayed_datagram_info *dg_info;

			if (atomic_add_return(1, &delayed_dg_host_queue_size)
			    == VMCI_MAX_DELAYED_DG_HOST_QUEUE_SIZE) {
				atomic_dec(&delayed_dg_host_queue_size);
				vmci_resource_put(resource);
				return VMCI_ERROR_NO_MEM;
			}

			dg_info = kmalloc(sizeof(*dg_info) +
				    (size_t) dg->payload_size, GFP_ATOMIC);
			if (!dg_info) {
				atomic_dec(&delayed_dg_host_queue_size);
				vmci_resource_put(resource);
				return VMCI_ERROR_NO_MEM;
			}

			dg_info->in_dg_host_queue = true;
			dg_info->entry = dst_entry;
			memcpy(&dg_info->msg, dg, dg_size);

			INIT_WORK(&dg_info->work, dg_delayed_dispatch);
			schedule_work(&dg_info->work);
			retval = VMCI_SUCCESS;

		} else {
			retval = dst_entry->recv_cb(dst_entry->client_data, dg);
			vmci_resource_put(resource);
			if (retval < VMCI_SUCCESS)
				return retval;
		}
	} else {
		/* Route to destination VM context. */
		struct vmci_datagram *new_dg;

		if (context_id != dg->dst.context) {
			if (vmci_deny_interaction(src_priv_flags,
						  vmci_context_get_priv_flags
						  (dg->dst.context))) {
				return VMCI_ERROR_NO_ACCESS;
			} else if (VMCI_CONTEXT_IS_VM(context_id)) {
				/*
				 * If the sending context is a VM, it
				 * cannot reach another VM.
				 */

				pr_devel("Datagram communication between VMs not supported (src=0x%x, dst=0x%x)\n",
					 context_id, dg->dst.context);
				return VMCI_ERROR_DST_UNREACHABLE;
			}
		}

		/* We make a copy to enqueue. */
		new_dg = kmemdup(dg, dg_size, GFP_KERNEL);
		if (new_dg == NULL)
			return VMCI_ERROR_NO_MEM;

		retval = vmci_ctx_enqueue_datagram(dg->dst.context, new_dg);
		if (retval < VMCI_SUCCESS) {
			kfree(new_dg);
			return retval;
		}
	}

	/*
	 * We currently truncate the size to signed 32 bits. This doesn't
	 * matter for this handler as it only support 4Kb messages.
	 */
	return (int)dg_size;
}
