int mlx4_ib_query_ah(struct ib_ah *ibah, struct ib_ah_attr *ah_attr)
{
	struct mlx4_ib_ah *ah = to_mah(ibah);
	enum rdma_link_layer ll;

	memset(ah_attr, 0, sizeof *ah_attr);
	ah_attr->port_num = be32_to_cpu(ah->av.ib.port_pd) >> 24;
	ll = rdma_port_get_link_layer(ibah->device, ah_attr->port_num);
	if (ll == IB_LINK_LAYER_ETHERNET)
		ah_attr->sl = be32_to_cpu(ah->av.eth.sl_tclass_flowlabel) >> 29;
	else
		ah_attr->sl = be32_to_cpu(ah->av.ib.sl_tclass_flowlabel) >> 28;

	ah_attr->dlid = ll == IB_LINK_LAYER_INFINIBAND ? be16_to_cpu(ah->av.ib.dlid) : 0;
	if (ah->av.ib.stat_rate)
		ah_attr->static_rate = ah->av.ib.stat_rate - MLX4_STAT_RATE_OFFSET;
	ah_attr->src_path_bits = ah->av.ib.g_slid & 0x7F;

	if (mlx4_ib_ah_grh_present(ah)) {
		ah_attr->ah_flags = IB_AH_GRH;

		ah_attr->grh.traffic_class =
			be32_to_cpu(ah->av.ib.sl_tclass_flowlabel) >> 20;
		ah_attr->grh.flow_label =
			be32_to_cpu(ah->av.ib.sl_tclass_flowlabel) & 0xfffff;
		ah_attr->grh.hop_limit  = ah->av.ib.hop_limit;
		ah_attr->grh.sgid_index = ah->av.ib.gid_index;
		memcpy(ah_attr->grh.dgid.raw, ah->av.ib.dgid, 16);
	}

	return 0;
}
