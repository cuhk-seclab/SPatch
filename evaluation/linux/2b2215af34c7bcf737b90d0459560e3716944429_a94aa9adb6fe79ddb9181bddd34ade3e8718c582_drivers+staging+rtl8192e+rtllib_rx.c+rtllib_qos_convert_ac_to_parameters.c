static int rtllib_qos_convert_ac_to_parameters(struct rtllib_qos_parameter_info *param_elm,
					       struct rtllib_qos_data *qos_data)
{
	struct rtllib_qos_ac_parameter *ac_params;
	struct rtllib_qos_parameters *qos_param = &(qos_data->parameters);
	int i;
	u8 aci;
	u8 acm;

	qos_data->wmm_acm = 0;
	for (i = 0; i < QOS_QUEUE_NUM; i++) {
		ac_params = &(param_elm->ac_params_record[i]);

		aci = (ac_params->aci_aifsn & 0x60) >> 5;
		acm = (ac_params->aci_aifsn & 0x10) >> 4;

		if (aci >= QOS_QUEUE_NUM)
			continue;
		switch (aci) {
		case 1:
			/* BIT(0) | BIT(3) */
			if (acm)
				qos_data->wmm_acm |= (0x01<<0)|(0x01<<3);
			break;
		case 2:
			/* BIT(4) | BIT(5) */
			if (acm)
				qos_data->wmm_acm |= (0x01<<4)|(0x01<<5);
			break;
		case 3:
			/* BIT(6) | BIT(7) */
			if (acm)
				qos_data->wmm_acm |= (0x01<<6)|(0x01<<7);
			break;
		case 0:
		default:
			/* BIT(1) | BIT(2) */
			if (acm)
				qos_data->wmm_acm |= (0x01<<1)|(0x01<<2);
			break;
		}

		qos_param->aifs[aci] = (ac_params->aci_aifsn) & 0x0f;

		/* WMM spec P.11: The minimum value for AIFSN shall be 2 */
		qos_param->aifs[aci] = (qos_param->aifs[aci] < 2) ? 2 : qos_param->aifs[aci];

		qos_param->cw_min[aci] = cpu_to_le16(ac_params->ecw_min_max &
						     0x0F);

		qos_param->cw_max[aci] = cpu_to_le16((ac_params->ecw_min_max &
						      0xF0) >> 4);

		qos_param->flag[aci] =
		    (ac_params->aci_aifsn & 0x10) ? 0x01 : 0x00;
		qos_param->tx_op_limit[aci] = ac_params->tx_op_limit;
	}
	return 0;
}
