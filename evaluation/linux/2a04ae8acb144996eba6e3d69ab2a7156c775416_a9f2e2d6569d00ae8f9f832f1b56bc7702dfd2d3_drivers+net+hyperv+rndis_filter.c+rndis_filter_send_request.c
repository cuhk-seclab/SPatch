static int rndis_filter_send_request(struct rndis_device *dev,
				  struct rndis_request *req)
{
	int ret;
	struct hv_netvsc_packet *packet;
	struct hv_page_buffer page_buf[2];
	struct hv_page_buffer *pb = page_buf;

	/* Setup the packet to send it */
	packet = &req->pkt;

	packet->is_data_pkt = false;
	packet->total_data_buflen = req->request_msg.msg_len;
	packet->page_buf_cnt = 1;

	pb[0].pfn = virt_to_phys(&req->request_msg) >>
					PAGE_SHIFT;
	pb[0].len = req->request_msg.msg_len;
	pb[0].offset =
		(unsigned long)&req->request_msg & (PAGE_SIZE - 1);

	/* Add one page_buf when request_msg crossing page boundary */
	if (pb[0].offset + pb[0].len > PAGE_SIZE) {
		packet->page_buf_cnt++;
		pb[0].len = PAGE_SIZE -
			pb[0].offset;
		pb[1].pfn = virt_to_phys((void *)&req->request_msg
			+ pb[0].len) >> PAGE_SHIFT;
		pb[1].offset = 0;
		pb[1].len = req->request_msg.msg_len -
			pb[0].len;
	}

	packet->completion_func = 0;
	packet->xmit_more = false;

	ret = netvsc_send(dev->net_dev->dev, packet, NULL, &pb);
	return ret;
}
