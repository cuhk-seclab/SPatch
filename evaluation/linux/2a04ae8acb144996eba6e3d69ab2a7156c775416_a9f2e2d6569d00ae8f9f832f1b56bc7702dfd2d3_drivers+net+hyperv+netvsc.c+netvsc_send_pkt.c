static inline int netvsc_send_pkt(
	struct hv_netvsc_packet *packet,
	struct netvsc_device *net_device,
	struct hv_page_buffer **pb)
{
	struct nvsp_message nvmsg;
	u16 q_idx = packet->q_idx;
	struct vmbus_channel *out_channel = net_device->chn_table[q_idx];
	struct net_device *ndev = net_device->ndev;
	u64 req_id;
	int ret;
	struct hv_page_buffer *pgbuf;
	u32 ring_avail = hv_ringbuf_avail_percent(&out_channel->outbound);

	nvmsg.hdr.msg_type = NVSP_MSG1_TYPE_SEND_RNDIS_PKT;
	if (packet->is_data_pkt) {
		/* 0 is RMC_DATA; */
		nvmsg.msg.v1_msg.send_rndis_pkt.channel_type = 0;
	} else {
		/* 1 is RMC_CONTROL; */
		nvmsg.msg.v1_msg.send_rndis_pkt.channel_type = 1;
	}

	nvmsg.msg.v1_msg.send_rndis_pkt.send_buf_section_index =
		packet->send_buf_index;
	if (packet->send_buf_index == NETVSC_INVALID_INDEX)
		nvmsg.msg.v1_msg.send_rndis_pkt.send_buf_section_size = 0;
	else
		nvmsg.msg.v1_msg.send_rndis_pkt.send_buf_section_size =
			packet->total_data_buflen;

	if (packet->completion_func)
		req_id = (ulong)packet;
	else
		req_id = 0;

	if (out_channel->rescind)
		return -ENODEV;

	/*
	 * It is possible that once we successfully place this packet
	 * on the ringbuffer, we may stop the queue. In that case, we want
	 * to notify the host independent of the xmit_more flag. We don't
	 * need to be precise here; in the worst case we may signal the host
	 * unnecessarily.
	 */
	if (ring_avail < (RING_AVAIL_PERCENT_LOWATER + 1))
		packet->xmit_more = false;

	if (packet->page_buf_cnt) {
		pgbuf = packet->cp_partial ? (*pb) +
			packet->rmsg_pgcnt : (*pb);
		ret = vmbus_sendpacket_pagebuffer_ctl(out_channel,
						      pgbuf,
						      packet->page_buf_cnt,
						      &nvmsg,
						      sizeof(struct nvsp_message),
						      req_id,
						      VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED,
						      !packet->xmit_more);
	} else {
		ret = vmbus_sendpacket_ctl(out_channel, &nvmsg,
					   sizeof(struct nvsp_message),
					   req_id,
					   VM_PKT_DATA_INBAND,
					   VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED,
					   !packet->xmit_more);
	}

	if (ret == 0) {
		atomic_inc(&net_device->num_outstanding_sends);
		atomic_inc(&net_device->queue_sends[q_idx]);

		if (ring_avail < RING_AVAIL_PERCENT_LOWATER) {
			netif_tx_stop_queue(netdev_get_tx_queue(ndev, q_idx));

			if (atomic_read(&net_device->
				queue_sends[q_idx]) < 1)
				netif_tx_wake_queue(netdev_get_tx_queue(
						    ndev, q_idx));
		}
	} else if (ret == -EAGAIN) {
		netif_tx_stop_queue(netdev_get_tx_queue(
				    ndev, q_idx));
		if (atomic_read(&net_device->queue_sends[q_idx]) < 1) {
			netif_tx_wake_queue(netdev_get_tx_queue(
					    ndev, q_idx));
			ret = -ENOSPC;
		}
	} else {
		netdev_err(ndev, "Unable to send packet %p ret %d\n",
			   packet, ret);
	}

	return ret;
}
