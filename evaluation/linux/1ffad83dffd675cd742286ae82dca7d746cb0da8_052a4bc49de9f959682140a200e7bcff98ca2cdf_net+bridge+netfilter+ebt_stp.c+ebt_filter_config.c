static bool ebt_filter_config(const struct ebt_stp_info *info,
			      const struct stp_config_pdu *stpc)
{
	const struct ebt_stp_config_info *c;
	uint16_t v16;
	uint32_t v32;
	int verdict, i;

	c = &info->config;
	if ((info->bitmask & EBT_STP_FLAGS) &&
	    FWINV(c->flags != stpc->flags, EBT_STP_FLAGS))
		return false;
	if (info->bitmask & EBT_STP_ROOTPRIO) {
		v16 = NR16(stpc->root);
		if (FWINV(v16 < c->root_priol ||
		    v16 > c->root_priou, EBT_STP_ROOTPRIO))
			return false;
	}
	if (info->bitmask & EBT_STP_ROOTADDR) {
		verdict = 0;
		for (i = 0; i < 6; i++)
			verdict |= (stpc->root[2+i] ^ c->root_addr[i]) &
				   c->root_addrmsk[i];
		if (FWINV(verdict != 0, EBT_STP_ROOTADDR))
			return false;
	}
	if (info->bitmask & EBT_STP_ROOTCOST) {
		v32 = NR32(stpc->root_cost);
		if (FWINV(v32 < c->root_costl ||
		    v32 > c->root_costu, EBT_STP_ROOTCOST))
			return false;
	}
	if (info->bitmask & EBT_STP_SENDERPRIO) {
		v16 = NR16(stpc->sender);
		if (FWINV(v16 < c->sender_priol ||
		    v16 > c->sender_priou, EBT_STP_SENDERPRIO))
			return false;
	}
	if (info->bitmask & EBT_STP_SENDERADDR) {
		verdict = 0;
		for (i = 0; i < 6; i++)
			verdict |= (stpc->sender[2+i] ^ c->sender_addr[i]) &
				   c->sender_addrmsk[i];
		if (FWINV(verdict != 0, EBT_STP_SENDERADDR))
			return false;
	}
	if (info->bitmask & EBT_STP_PORT) {
		v16 = NR16(stpc->port);
		if (FWINV(v16 < c->portl ||
		    v16 > c->portu, EBT_STP_PORT))
			return false;
	}
	if (info->bitmask & EBT_STP_MSGAGE) {
		v16 = NR16(stpc->msg_age);
		if (FWINV(v16 < c->msg_agel ||
		    v16 > c->msg_ageu, EBT_STP_MSGAGE))
			return false;
	}
	if (info->bitmask & EBT_STP_MAXAGE) {
		v16 = NR16(stpc->max_age);
		if (FWINV(v16 < c->max_agel ||
		    v16 > c->max_ageu, EBT_STP_MAXAGE))
			return false;
	}
	if (info->bitmask & EBT_STP_HELLOTIME) {
		v16 = NR16(stpc->hello_time);
		if (FWINV(v16 < c->hello_timel ||
		    v16 > c->hello_timeu, EBT_STP_HELLOTIME))
			return false;
	}
	if (info->bitmask & EBT_STP_FWDD) {
		v16 = NR16(stpc->forward_delay);
		if (FWINV(v16 < c->forward_delayl ||
		    v16 > c->forward_delayu, EBT_STP_FWDD))
			return false;
	}
	return true;
}
