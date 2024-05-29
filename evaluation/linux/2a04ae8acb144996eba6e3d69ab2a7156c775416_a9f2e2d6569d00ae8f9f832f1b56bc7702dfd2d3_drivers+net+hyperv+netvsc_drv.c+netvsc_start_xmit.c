static int netvsc_start_xmit(struct sk_buff *skb, struct net_device *net)
{
	struct net_device_context *net_device_ctx = netdev_priv(net);
	struct hv_netvsc_packet *packet = NULL;
	int ret;
	unsigned int num_data_pgs;
	struct rndis_message *rndis_msg;
	struct rndis_packet *rndis_pkt;
	u32 rndis_msg_size;
	bool isvlan;
	bool linear = false;
	struct rndis_per_packet_info *ppi;
	struct ndis_tcp_ip_checksum_info *csum_info;
	struct ndis_tcp_lso_info *lso_info;
	int  hdr_offset;
	u32 net_trans_info;
	u32 hash;
	u32 skb_length;
	struct hv_page_buffer page_buf[MAX_PAGE_BUFFER_COUNT];
	struct hv_page_buffer *pb = page_buf;
	struct netvsc_stats *tx_stats = this_cpu_ptr(net_device_ctx->tx_stats);

	/* We will atmost need two pages to describe the rndis
	 * header. We can only transmit MAX_PAGE_BUFFER_COUNT number
	 * of pages in a single packet. If skb is scattered around
	 * more pages we try linearizing it.
	 */

check_size:
	skb_length = skb->len;
	num_data_pgs = netvsc_get_slots(skb) + 2;
	if (num_data_pgs > MAX_PAGE_BUFFER_COUNT && linear) {
		net_alert_ratelimited("packet too big: %u pages (%u bytes)\n",
				      num_data_pgs, skb->len);
		ret = -EFAULT;
		goto drop;
	} else if (num_data_pgs > MAX_PAGE_BUFFER_COUNT) {
		if (skb_linearize(skb)) {
			net_alert_ratelimited("failed to linearize skb\n");
			ret = -ENOMEM;
			goto drop;
		}
		linear = true;
		goto check_size;
	}

	/*
	 * Place the rndis header in the skb head room and
	 * the skb->cb will be used for hv_netvsc_packet
	 * structure.
	 */
	ret = skb_cow_head(skb, RNDIS_AND_PPI_SIZE);
	if (ret) {
		netdev_err(net, "unable to alloc hv_netvsc_packet\n");
		ret = -ENOMEM;
		goto drop;
	}
	/* Use the skb control buffer for building up the packet */
	BUILD_BUG_ON(sizeof(struct hv_netvsc_packet) >
			FIELD_SIZEOF(struct sk_buff, cb));
	packet = (struct hv_netvsc_packet *)skb->cb;

	packet->status = 0;
	packet->xmit_more = skb->xmit_more;

	packet->vlan_tci = skb->vlan_tci;

	packet->q_idx = skb_get_queue_mapping(skb);

	packet->is_data_pkt = true;
	packet->total_data_buflen = skb->len;

	rndis_msg = (struct rndis_message *)skb->head;

	memset(rndis_msg, 0, RNDIS_AND_PPI_SIZE);

	/* Set the completion routine */
	packet->completion_func = 1;
	packet->send_completion_tid = (unsigned long)skb;

	isvlan = packet->vlan_tci & VLAN_TAG_PRESENT;

	/* Add the rndis header */
	rndis_msg->ndis_msg_type = RNDIS_MSG_PACKET;
	rndis_msg->msg_len = packet->total_data_buflen;
	rndis_pkt = &rndis_msg->msg.pkt;
	rndis_pkt->data_offset = sizeof(struct rndis_packet);
	rndis_pkt->data_len = packet->total_data_buflen;
	rndis_pkt->per_pkt_info_offset = sizeof(struct rndis_packet);

	rndis_msg_size = RNDIS_MESSAGE_SIZE(struct rndis_packet);

	hash = skb_get_hash_raw(skb);
	if (hash != 0 && net->real_num_tx_queues > 1) {
		rndis_msg_size += NDIS_HASH_PPI_SIZE;
		ppi = init_ppi_data(rndis_msg, NDIS_HASH_PPI_SIZE,
				    NBL_HASH_VALUE);
		*(u32 *)((void *)ppi + ppi->ppi_offset) = hash;
	}

	if (isvlan) {
		struct ndis_pkt_8021q_info *vlan;

		rndis_msg_size += NDIS_VLAN_PPI_SIZE;
		ppi = init_ppi_data(rndis_msg, NDIS_VLAN_PPI_SIZE,
					IEEE_8021Q_INFO);
		vlan = (struct ndis_pkt_8021q_info *)((void *)ppi +
						ppi->ppi_offset);
		vlan->vlanid = packet->vlan_tci & VLAN_VID_MASK;
		vlan->pri = (packet->vlan_tci & VLAN_PRIO_MASK) >>
				VLAN_PRIO_SHIFT;
	}

	net_trans_info = get_net_transport_info(skb, &hdr_offset);
	if (net_trans_info == TRANSPORT_INFO_NOT_IP)
		goto do_send;

	/*
	 * Setup the sendside checksum offload only if this is not a
	 * GSO packet.
	 */
	if (skb_is_gso(skb))
		goto do_lso;

	if ((skb->ip_summed == CHECKSUM_NONE) ||
	    (skb->ip_summed == CHECKSUM_UNNECESSARY))
		goto do_send;

	rndis_msg_size += NDIS_CSUM_PPI_SIZE;
	ppi = init_ppi_data(rndis_msg, NDIS_CSUM_PPI_SIZE,
			    TCPIP_CHKSUM_PKTINFO);

	csum_info = (struct ndis_tcp_ip_checksum_info *)((void *)ppi +
			ppi->ppi_offset);

	if (net_trans_info & (INFO_IPV4 << 16))
		csum_info->transmit.is_ipv4 = 1;
	else
		csum_info->transmit.is_ipv6 = 1;

	if (net_trans_info & INFO_TCP) {
		csum_info->transmit.tcp_checksum = 1;
		csum_info->transmit.tcp_header_offset = hdr_offset;
	} else if (net_trans_info & INFO_UDP) {
		/* UDP checksum offload is not supported on ws2008r2.
		 * Furthermore, on ws2012 and ws2012r2, there are some
		 * issues with udp checksum offload from Linux guests.
		 * (these are host issues).
		 * For now compute the checksum here.
		 */
		struct udphdr *uh;
		u16 udp_len;

		ret = skb_cow_head(skb, 0);
		if (ret)
			goto drop;

		uh = udp_hdr(skb);
		udp_len = ntohs(uh->len);
		uh->check = 0;
		uh->check = csum_tcpudp_magic(ip_hdr(skb)->saddr,
					      ip_hdr(skb)->daddr,
					      udp_len, IPPROTO_UDP,
					      csum_partial(uh, udp_len, 0));
		if (uh->check == 0)
			uh->check = CSUM_MANGLED_0;

		csum_info->transmit.udp_checksum = 0;
	}
	goto do_send;

do_lso:
	rndis_msg_size += NDIS_LSO_PPI_SIZE;
	ppi = init_ppi_data(rndis_msg, NDIS_LSO_PPI_SIZE,
			    TCP_LARGESEND_PKTINFO);

	lso_info = (struct ndis_tcp_lso_info *)((void *)ppi +
			ppi->ppi_offset);

	lso_info->lso_v2_transmit.type = NDIS_TCP_LARGE_SEND_OFFLOAD_V2_TYPE;
	if (net_trans_info & (INFO_IPV4 << 16)) {
		lso_info->lso_v2_transmit.ip_version =
			NDIS_TCP_LARGE_SEND_OFFLOAD_IPV4;
		ip_hdr(skb)->tot_len = 0;
		ip_hdr(skb)->check = 0;
		tcp_hdr(skb)->check =
		~csum_tcpudp_magic(ip_hdr(skb)->saddr,
				   ip_hdr(skb)->daddr, 0, IPPROTO_TCP, 0);
	} else {
		lso_info->lso_v2_transmit.ip_version =
			NDIS_TCP_LARGE_SEND_OFFLOAD_IPV6;
		ipv6_hdr(skb)->payload_len = 0;
		tcp_hdr(skb)->check =
		~csum_ipv6_magic(&ipv6_hdr(skb)->saddr,
				&ipv6_hdr(skb)->daddr, 0, IPPROTO_TCP, 0);
	}
	lso_info->lso_v2_transmit.tcp_header_offset = hdr_offset;
	lso_info->lso_v2_transmit.mss = skb_shinfo(skb)->gso_size;

do_send:
	/* Start filling in the page buffers with the rndis hdr */
	rndis_msg->msg_len += rndis_msg_size;
	packet->total_data_buflen = rndis_msg->msg_len;
	packet->page_buf_cnt = init_page_array(rndis_msg, rndis_msg_size,
					       skb, packet, &pb);

	ret = netvsc_send(net_device_ctx->device_ctx, packet, rndis_msg, &pb);

drop:
	if (ret == 0) {
		u64_stats_update_begin(&tx_stats->syncp);
		tx_stats->packets++;
		tx_stats->bytes += skb_length;
		u64_stats_update_end(&tx_stats->syncp);
	} else {
		if (ret != -EAGAIN) {
			dev_kfree_skb_any(skb);
			net->stats.tx_dropped++;
		}
	}

	return (ret == -EAGAIN) ? NETDEV_TX_BUSY : NETDEV_TX_OK;
}
