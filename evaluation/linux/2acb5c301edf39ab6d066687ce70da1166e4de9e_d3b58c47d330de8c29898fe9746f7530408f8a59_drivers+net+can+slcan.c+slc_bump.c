static void slc_bump(struct slcan *sl)
{
	struct sk_buff *skb;
	struct can_frame cf;
	int i, tmp;
	u32 tmpid;
	char *cmd = sl->rbuff;

	cf.can_id = 0;

	switch (*cmd) {
	case 'r':
		cf.can_id = CAN_RTR_FLAG;
		/* fallthrough */
	case 't':
		/* store dlc ASCII value and terminate SFF CAN ID string */
		cf.can_dlc = sl->rbuff[SLC_CMD_LEN + SLC_SFF_ID_LEN];
		sl->rbuff[SLC_CMD_LEN + SLC_SFF_ID_LEN] = 0;
		/* point to payload data behind the dlc */
		cmd += SLC_CMD_LEN + SLC_SFF_ID_LEN + 1;
		break;
	case 'R':
		cf.can_id = CAN_RTR_FLAG;
		/* fallthrough */
	case 'T':
		cf.can_id |= CAN_EFF_FLAG;
		/* store dlc ASCII value and terminate EFF CAN ID string */
		cf.can_dlc = sl->rbuff[SLC_CMD_LEN + SLC_EFF_ID_LEN];
		sl->rbuff[SLC_CMD_LEN + SLC_EFF_ID_LEN] = 0;
		/* point to payload data behind the dlc */
		cmd += SLC_CMD_LEN + SLC_EFF_ID_LEN + 1;
		break;
	default:
		return;
	}

	if (kstrtou32(sl->rbuff + SLC_CMD_LEN, 16, &tmpid))
		return;

	cf.can_id |= tmpid;

	/* get can_dlc from sanitized ASCII value */
	if (cf.can_dlc >= '0' && cf.can_dlc < '9')
		cf.can_dlc -= '0';
	else
		return;

	*(u64 *) (&cf.data) = 0; /* clear payload */

	/* RTR frames may have a dlc > 0 but they never have any data bytes */
	if (!(cf.can_id & CAN_RTR_FLAG)) {
		for (i = 0; i < cf.can_dlc; i++) {
			tmp = hex_to_bin(*cmd++);
			if (tmp < 0)
				return;
			cf.data[i] = (tmp << 4);
			tmp = hex_to_bin(*cmd++);
			if (tmp < 0)
				return;
			cf.data[i] |= tmp;
		}
	}

	skb = dev_alloc_skb(sizeof(struct can_frame) +
			    sizeof(struct can_skb_priv));
	if (!skb)
		return;

	skb->dev = sl->dev;
	skb->protocol = htons(ETH_P_CAN);
	skb->pkt_type = PACKET_BROADCAST;
	skb->ip_summed = CHECKSUM_UNNECESSARY;

	can_skb_reserve(skb);
	can_skb_prv(skb)->ifindex = sl->dev->ifindex;
	can_skb_prv(skb)->skbcnt = 0;

	memcpy(skb_put(skb, sizeof(struct can_frame)),
	       &cf, sizeof(struct can_frame));
	netif_rx_ni(skb);

	sl->dev->stats.rx_packets++;
	sl->dev->stats.rx_bytes += cf.can_dlc;
}
